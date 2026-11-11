"""Compatibility entry for the current R2 package gate."""
import runpy
from pathlib import Path
if __name__ == "__main__":
 runpy.run_path(str(Path(__file__).with_name("test_r2_package.py")),run_name="__main__")
