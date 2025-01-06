
import os
import sys
import tempfile
from utilities.common import run_command
from utilities.files import path_join

vcvarsall_env_created = False

def call_vcvarsall(root_dir: str, args: list):
    global vcvarsall_env_created

    # If first time, then create a temp/vcvarsall_env.txt file
    # with all the current env variables.
    #
    # If this function found that temp/vcvarsall_env.txt exsists, then
    # it will restore the env from that file before calling vcvarsall.bat
    #
    # This is needed to avoid PATH pollution with each subsequent call 
    # to vcvarsall.bat
    vcvarsall_env_file = path_join(root_dir, "temp", "vcvarsall_env.txt")
    try:
        if not vcvarsall_env_created:
            # Delete the file if it exists
            if os.path.exists(vcvarsall_env_file):
                os.remove(vcvarsall_env_file)
            with open(vcvarsall_env_file, 'w') as f:
                for key, value in os.environ.items():
                    f.write(f'{key}={value}\n')
            # Verify that vcvarsall_env_file exists.
            if os.path.exists(vcvarsall_env_file):
                vcvarsall_env_created = True
        else:
            with open(vcvarsall_env_file, 'r') as f:
                for line in f:
                    if line.strip():
                        key, value = line.strip().split('=', 1)
                        os.environ[key] = value
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

    if not sys.platform == "win32":
        print(f"Unexpected call to vcvarsall. Not supported on this platform")
        sys.exit(1)
    
    vswhere_path = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if not os.path.exists(vswhere_path):
        print(f"Error: vswhere.exe not found at {vswhere_path}")
        sys.exit(1)

    # Print out the PATH variable
    #print(f"PATH before vcvarsall: {os.environ['PATH']}")

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

            # Print out the PATH variable
            #print(f"PATH after vcvarsall: {os.environ['PATH']}")

        except Exception as e:
            print(f"Error: {e}")
            sys.exit(1)

    # Verify that platform is set.
    if not os.environ.get('Platform', None):
        print(f"Error: Platform not set by vcvarsall.bat")
        sys.exit(1)
    print(f"vcvarsall.bat called successfully")
    print(f"Platform: {os.environ['Platform']}")
