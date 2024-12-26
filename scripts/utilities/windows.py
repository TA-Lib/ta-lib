
import os
import sys
import tempfile
from utilities.common import run_command
from utilities.files import path_join


def call_vcvarsall(args: list):
    if not sys.platform == "win32":
        print(f"Unexpected call to vcvarsall. Not supported on this platform")
        sys.exit(1)
    
    vswhere_path = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if not os.path.exists(vswhere_path):
        print(f"Error: vswhere.exe not found at {vswhere_path}")
        sys.exit(1)

    vswhere_command = [vswhere_path, "-latest", "-products", "*", "-requires", "Microsoft.Component.MSBuild", "-find", "VC/Auxiliary/Build/vcvarsall.bat"]
    vcvarsall_path = run_command(vswhere_command)
    if not os.path.exists(vcvarsall_path):
        print(f"Error: vcvarsall.bat not found at {vcvarsall_path}")
        sys.exit(1)
    # Create a temporary batch file to call vcvarsall.bat and save a copy
    # of the whole env, so we can re-inject these into this python process.
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_batch_file = path_join(temp_dir, "temp_vcvarsall.bat")
        temp_env_file = path_join(temp_dir, "temp_env.txt")
        try:
            with open(temp_batch_file, 'w') as f:
                args_str = ' '.join(args)
                f.write(f'call "{vcvarsall_path}" {args_str}\n')
                f.write(f'set > "{temp_env_file}"\n')
            # Execute the temporary batch file, reinject the env.
            run_command([temp_batch_file])    
            with open(temp_env_file, 'r') as f:
                for line in f:
                    if line.strip():
                        key, value = line.strip().split('=', 1)
                        os.environ[key] = value
        except Exception as e:
            print(f"Error: {e}")
            sys.exit(1)

    # Verify that platform is set.
    if not os.environ.get('Platform', None):
        print(f"Error: Platform not set by vcvarsall.bat")
        sys.exit(1)
    print(f"vcvarsall.bat called successfully")
    print(f"Platform: {os.environ['Platform']}")
