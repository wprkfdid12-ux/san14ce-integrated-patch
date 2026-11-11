@echo off
cd /d "%~dp0"
for %%f in (SAN14CE_Patch_UI_R4_9.exe SAN14CE_Patch_UI_R4_9_PLUS_22_25.exe SAN14CE_Patch_Setup.cmd SAN14CE_Patch_UI_R4_9_PLUS_22_25.exe.BUILD_ARGS.txt SAN14CE_Patch_UI_R4_9_PLUS_22_25.exe.BUILD_LOG.txt) do if exist "%%f" (del /q "%%f" & echo removed %%f)
if exist "support\extras\26" (rmdir /s /q "support\extras\26" & echo removed support\extras\26)
if exist "support\recovery_r8\SAN14_LegionCommands.dll" (del /q "support\recovery_r8\SAN14_LegionCommands.dll" & echo removed support\recovery_r8\SAN14_LegionCommands.dll)
echo done.
pause
