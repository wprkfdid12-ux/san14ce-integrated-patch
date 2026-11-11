"""Small dependency-free PE32+ reader for this build and its structural checks.
Not a replacement for a general executable parser. Rejects malformed ranges.
"""
import struct
from types import SimpleNamespace

class Struct(SimpleNamespace):
    def get_file_offset(self): return self._offset
    def get_field_absolute_offset(self,name): return self._offset+self._fields[name]

class Section(Struct):
    def get_data(self): return self._pe.__data__[self.PointerToRawData:self.PointerToRawData+self.SizeOfRawData]

class PE:
    def __init__(self,name=None,*,data=None,fast_load=False):
        self.__data__=bytes(data if data is not None else open(name,'rb').read());d=self.__data__
        def u(fmt,o): return struct.unpack_from('<'+fmt,d,o)
        assert len(d)>=64 and d[:2]==b'MZ'
        pos=u('I',0x3c)[0];assert d[pos:pos+4]==b'PE\0\0'
        machine,n,stamp,syms,ns,sz,flags=u('HHIIIHH',pos+4)
        self.FILE_HEADER=Struct(_offset=pos+4,_fields={'NumberOfSections':2,'Characteristics':18},Machine=machine,NumberOfSections=n,TimeDateStamp=stamp,Characteristics=flags,SizeOfOptionalHeader=sz)
        o=pos+24;assert u('H',o)[0]==0x20b
        fields={'SizeOfInitializedData':8,'AddressOfEntryPoint':16,'ImageBase':24,'SectionAlignment':32,'FileAlignment':36,'SizeOfImage':56,'SizeOfHeaders':60,'CheckSum':64,'DllCharacteristics':70,'NumberOfRvaAndSizes':108}
        vals={k:u('Q' if k=='ImageBase' else 'H' if k=='DllCharacteristics' else 'I',o+v)[0] for k,v in fields.items()}
        self.OPTIONAL_HEADER=Struct(_offset=o,_fields=fields,**vals)
        self.OPTIONAL_HEADER.DATA_DIRECTORY=[Struct(_offset=o+112+i*8,_fields={},VirtualAddress=u('I',o+112+i*8)[0],Size=u('I',o+116+i*8)[0]) for i in range(min(16,vals['NumberOfRvaAndSizes']))]
        self.sections=[]
        for i in range(n):
            so=o+sz+i*40
            name,vs,va,rs,rp,rrel,rline,nrel,nline,ch=u('8s6I2HI',so)
            assert rp+rs<=len(d) if rs else True
            self.sections.append(Section(_offset=so,_fields={},_pe=self,Name=name,Misc_VirtualSize=vs,VirtualAddress=va,SizeOfRawData=rs,PointerToRawData=rp,Characteristics=ch))
        self.DIRECTORY_ENTRY_IMPORT=[];self.DIRECTORY_ENTRY_EXPORT=Struct(symbols=[])
        self.DIRECTORY_ENTRY_EXCEPTION=[]
        if not fast_load:self._directories()
    def get_offset_from_rva(self,rva):
        if 0<=rva<self.OPTIONAL_HEADER.SizeOfHeaders:return rva
        for s in self.sections:
            if s.VirtualAddress<=rva<s.VirtualAddress+max(s.Misc_VirtualSize,s.SizeOfRawData):
                off=rva-s.VirtualAddress
                if off>=s.SizeOfRawData:raise ValueError('RVA lies in zero-fill region')
                return s.PointerToRawData+off
        raise ValueError('unmapped RVA '+hex(rva))
    def get_rva_from_offset(self,off):
        if off<self.OPTIONAL_HEADER.SizeOfHeaders:return off
        for s in self.sections:
            if s.PointerToRawData<=off<s.PointerToRawData+s.SizeOfRawData:return s.VirtualAddress+off-s.PointerToRawData
        raise ValueError('unmapped offset')
    def get_data(self,rva,n):
        off=self.get_offset_from_rva(rva);assert off+n<=len(self.__data__)
        return self.__data__[off:off+n]
    def string(self,rva):
        off=self.get_offset_from_rva(rva);end=self.__data__.find(b'\0',off)
        if end<0:raise ValueError('unterminated string')
        return self.__data__[off:end]
    def _directories(self):
        dirs=self.OPTIONAL_HEADER.DATA_DIRECTORY
        if len(dirs)>1 and dirs[1].VirtualAddress:
            base=dirs[1].VirtualAddress
            for i in range(256):
                orig,ts,fwd,name,iat=struct.unpack('<5I',self.get_data(base+20*i,20))
                if not any((orig,ts,fwd,name,iat)):break
                imports=[]
                for j in range(1024):
                    thunk=struct.unpack('<Q',self.get_data((orig or iat)+8*j,8))[0]
                    if not thunk:break
                    if thunk>>63:imports.append(Struct(name=None,ordinal=thunk&0xffff))
                    else:imports.append(Struct(name=self.string(thunk+2),address=self.OPTIONAL_HEADER.ImageBase+iat+j*8))
                self.DIRECTORY_ENTRY_IMPORT.append(Struct(dll=self.string(name),imports=imports))
        if dirs and dirs[0].VirtualAddress:
            v=struct.unpack('<IIHH7I',self.get_data(dirs[0].VirtualAddress,40))
            base,nf,nn,fa,na,oa=v[5:]
            syms=[]
            for i in range(nn):
                nrva=struct.unpack('<I',self.get_data(na+i*4,4))[0]
                ordidx=struct.unpack('<H',self.get_data(oa+i*2,2))[0]
                frva=struct.unpack('<I',self.get_data(fa+ordidx*4,4))[0]
                syms.append(Struct(name=self.string(nrva),address=frva,ordinal=base+ordidx))
            self.DIRECTORY_ENTRY_EXPORT=Struct(symbols=syms)
        if len(dirs)>3 and dirs[3].VirtualAddress:
            dd=dirs[3];assert dd.Size%12==0
            for i in range(dd.Size//12):
                start,end,unwind=struct.unpack('<III',self.get_data(dd.VirtualAddress+i*12,12))
                self.DIRECTORY_ENTRY_EXCEPTION.append(Struct(struct=Struct(BeginAddress=start,EndAddress=end,UnwindData=unwind)))
    def generate_checksum(self):
        data=bytearray(self.__data__);o=self.OPTIONAL_HEADER.get_field_absolute_offset('CheckSum')
        data[o:o+4]=b'\0'*4;n=len(data)
        if n%2:data.append(0)
        total=sum(x[0] for x in struct.iter_unpack('<H',data))
        while total>>16:total=(total&0xffff)+(total>>16)
        return (total+n)&0xffffffff
    def verify_checksum(self): return self.OPTIONAL_HEADER.CheckSum==self.generate_checksum()
