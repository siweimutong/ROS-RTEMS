#!/usr/bin/env python3
"""
SylixOS MK to RTEMS Wscript Converter (Static Libraries Version)
Convert all dynamic libraries to static library builds
"""

import re
import sys
import os


def parse_sylixos_mk(mk_file):
    """
    Full parser for SylixOS MK file format
    Return dict containing all build parameters
    Special handling: LOCAL_TARGET_NAME strips lib prefix and .so/.a suffix
    """
    config = {
        'target': None,          # Processed target name (lib prefix and .so/.a suffix removed)
        'raw_target': None,      # Raw target name
        'sources': [],           # Source file list
        'includes': [],          # Header search paths
        'defines': [],           # Predefined macros
        'cflags': [],            # C compile flags
        'cxxflags': [],          # C++ compile flags
        'ldflags': [],           # Link flags
        'libs': [],              # Dependency libraries
        'lib_paths': [],         # 库Search paths
        'depend_targets': [],    # Dependency targets
        'use_cxx': False,        # 是no使用C++
        'use_exceptions': False, # 是no启用异常
        'pre_link_cmd': None,    # Pre-link command
        'post_link_cmd': None    # Post-link command
    }

    current_var = None
    collecting_value = False
    value_lines = []

    def process_value(value_str, var_name):
        """Generic function to process variable values"""
        value_str = value_str.strip()
        if not value_str:
            return []
        
        # Special handling for different variable types
        if var_name == 'local_inc_path':
            # Process include paths (starting with -I or I)
            paths = []
            for item in re.split(r'(?<!\\)\s+', value_str):  # Split but preserve escaped spaces
                item = item.strip()
                if item.startswith('-I"'):
                    paths.append(item[3:-1])  # Strip -I" and trailing "
                elif item.startswith('I"'):
                    paths.append(item[2:-1])
                elif item.startswith('-I'):
                    paths.append(item[2:])
                elif item.startswith('I'):
                    paths.append(item[1:])
            return paths
        
        elif var_name == 'local_dsymbol':
            # Process macro definitions (starting with -D)
            return [item[2:] for item in re.split(r'(?<!\\)\s+', value_str) 
                    if item.startswith('-D')]
        
        elif var_name == 'local_depend_lib':
            # ProcessDependency libraries (-l 开头)
            return [item[2:] for item in re.split(r'(?<!\\)\s+', value_str) 
                    if item.startswith('-l')]
        
        elif var_name == 'local_depend_lib_path':
            # Process library paths (starting with -L)
            paths = []
            for item in re.split(r'(?<!\\)\s+', value_str):
                item = item.strip()
                if item.startswith('-L"'):
                    paths.append(item[3:-1])
                elif item.startswith('-L'):
                    paths.append(item[2:])
            return paths
        
        elif var_name == 'local_depend_target':
            # ProcessDependency targets (Strippath和扩展名)
            return [os.path.basename(item).split('.')[0].replace('lib', '') 
                   for item in re.split(r'(?<!\\)\s+', value_str) if item.strip()]
        
        # Default handling: split by spaces
        return [item for item in re.split(r'(?<!\\)\s+', value_str) if item.strip()]

    def process_target_name(target_name):
        """Process target name: strip lib prefix and .so/.a suffix"""
        if not target_name:
            return None
            
        # Save raw target name
        config['raw_target'] = target_name
        
        # Strip lib prefix
        if target_name.startswith('lib'):
            target_name = target_name[3:]
        
        # Strip .so or .a suffix
        if target_name.endswith('.so'):
            target_name = target_name[:-3]
        elif target_name.endswith('.a'):
            target_name = target_name[:-2]
        
        return target_name

    with open(mk_file, 'r', encoding='utf-8', errors='ignore') as f:
        flag = -1
        for line in f:
            line = line.strip()
            
            # Skipping注释和空Row
            if not line or line.startswith('#') or line.startswith('//'):
                if flag == 1:
                    collecting_value = False
                continue
            
            # Check if this is a variable definition line
            if ':=' in line and not collecting_value:
                # Start新的变量Define
                var_part, value_part = line.split(':=', 1)
                value_part = value_part.strip()
                
                # SpecialProcessTarget name
                if var_part.strip() == 'LOCAL_TARGET_NAME':
                    config['target'] = process_target_name(value_part)
                    continue
                elif var_part.strip() == 'LOCAL_SRCS':
                    collecting_value = True
                    current_var = var_part.strip().lower()
                    value_lines.append('init.c')
                    continue
                elif var_part.strip() == 'LOCAL_INC_PATH':
                    collecting_value = True
                    current_var = var_part.strip().lower()
                    continue
                elif var_part.strip() == 'LOCAL_CXXFLAGS':
                    current_var = var_part.strip().lower()
                    config[current_var] = value_part.strip()
                    continue
                elif var_part.strip() == 'LOCAL_DEPEND_TARGET':
                    collecting_value = True
                    flag = 1
                    current_var = var_part.strip().lower()
                    continue
            
            elif collecting_value:
                # Collecting multi-line values
                if line.endswith('\\'):
                    if "SYLIXOS_BASE_PATH" in line:
                        continue
                    if "$(Output)" in line:
                        line = line[18:]
                        line = line[:-1].strip()
                        line = process_target_name(line)
                        value_lines.append(line)
                        continue
                    if current_var == 'local_inc_path':
                        value_lines.append(line[3:-3].strip())
                        continue
                    value_lines.append(line[:-1].strip())
                else:
                    if "$(Output)" in line:
                        line = line[18:]
                        line = line.strip()
                        line = process_target_name(line)
                    if current_var == 'local_inc_path':
                        value_lines.append(line[3:-1])
                    else:
                        value_lines.append(line)
                    collecting_value = False
                    full_value = ' '.join(value_lines)
                    config[current_var] = value_lines
                    # config[current_var] = process_value(full_value, current_var)
                    value_lines = []
            
            # ProcessSpecial变量
            if line.startswith('LOCAL_USE_CXX :='):
                config['use_cxx'] = 'yes' in line.lower()
            elif line.startswith('LOCAL_USE_CXX_EXCEPT :='):
                config['use_exceptions'] = 'yes' in line.lower()
            elif line.startswith('LOCAL_PRE_LINK_CMD :='):
                config['pre_link_cmd'] = line.split(':=', 1)[1].strip('"\' ')
            elif line.startswith('LOCAL_POST_LINK_CMD :='):
                config['post_link_cmd'] = line.split(':=', 1)[1].strip('"\' ')

    # Post-processing: ensure key variables exist
    if not config['sources'] and 'local_srcs' in config:
        config['sources'] = config['local_srcs']
    
    if not config['includes'] and 'local_inc_path' in config:
        config['includes'] = config['local_inc_path']
    
    if not config['libs'] and 'local_depend_target' in config:
        config['libs'] = config['local_depend_target']
    
    if not config['cxxflags'] and 'local_cxxflags' in config:
        config['cxxflags'] = config['local_cxxflags']
    
    return config

def generate_rtems_wscript(config, output_file):
    """Generate RTEMS wscript file (static library version)"""
    lines = [
        "#!/usr/bin/env python",
        "# A Waf script to build an RTEMS project with static libraries",
        "",
        "from __future__ import print_function",
        "",
        'rtems_version = "6"',
        "",
        "try:",
        "    import rtems_waf.rtems as rtems",
        "except ImportError:",
        "    print('error: no rtems_waf git submodule')",
        "    import sys",
        "    sys.exit(1)",
        "",
        "def init(ctx):",
        "    rtems.init(ctx, version=rtems_version, long_commands=True)",
        "    ctx.load('compiler_cxx')  # Ensure C++ compiler is loaded",
        "",
        "def bsp_configure(conf, arch_bsp):",
        "    # 这里can以进Row BSP 相关的ConfigurationCheck",
        "    pass",
        "",
        "def options(opt):",
        "    rtems.options(opt)",
        "",
        "def configure(conf):",
        "    rtems.configure(conf, bsp_configure=bsp_configure)",
    ]

    # AddlibpathConfiguration
    if config['lib_paths']:
        lines.append("    ")
        lines.append("    # Add library paths")
        for path in config['lib_paths']:
            lines.append(f"    conf.env.append_value('libpath', '{path}')")
    
    lines.extend([
        "",
        "def build(bld):",
        "    rtems.build(bld)",
        "",
    ])

    # AddDependency targets
    if config['depend_targets']:
        lines.append("    # dependencytarget (static library)")
        for target in config['depend_targets']:
            lines.append(f"    bld.load('{target}')")
        lines.append("")

    # Main build target
    if config['target']:
        # target_name = config['target'].replace('.a', '')
        target_name = config['target']
        is_library = True
        
        lines.append(f"    # {config['target']} target (static library)")
        if is_library:
            lines.append("    # Compile and link static library")
            lines.append("    bld.stlib(")
            lines.append(f"        target='{target_name}',")
            
            # AddSource file
            if config['sources']:
                lines.append(f"        source={config['sources']},")
            
            # Add包含path
            if config['includes']:
                lines.append(f"        includes={config['includes']},")
            
            # AddCompile flags
            # cxxflags = list(set(config['cflags'] + config['cxxflags']))
            # res = []
            cxxflags = config['cxxflags']
            if '-fPIC' not in cxxflags:
                cxxflags += ' -fPIC'
            if '-fexceptions' not in cxxflags and config['use_exceptions']:
                cxxflags += ' -fexceptions'
            if cxxflags:
                # res.append(cxxflags)
                # cxxflags = '[' + cxxflags + ']'
                lines.append(f"        cxxflags='{cxxflags}',")
            
            # AddLink flags
            ldflags = list(set(config['ldflags'] + ['-static']))
            lines.append(f"        linkflags='{' '.join(ldflags)}',")
            
            # AddDependency libraries(移except.so后缀)
            all_libs = [lib.replace('.so', '') for lib in config['libs']] + ['stdc++', 'rtemscpu', 'rtemsbsp', 'rtemsdefaultconfig', 'm', 'gcc', 'c']
            lines.append(f"        lib={all_libs},")
            
            # Add library paths
            if config['lib_paths']:
                lines.append(f"        libpath={config['lib_paths']},")
            
            lines.append("    )")
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))

def convert_mk_to_wscript(mk_file, output_file=None):
    """Main conversion function"""
    if not os.path.exists(mk_file):
        print(f"Error: Input file {mk_file} not found!")
        return False

    if output_file is None:
        output_file = os.path.join(os.path.dirname(mk_file), "tmp/wscript")

    config = parse_sylixos_mk(mk_file)
    # for key, value in config.items():  
    #     print(f"{key}: {value}")  
    generate_rtems_wscript(config, output_file)

    print(f"Successfully converted {mk_file} to {output_file}")
    return True

def replace_src_with_source(file_path):
    """
    Replace 'src' with 'source' in source=[] and includes=[] lines
    
    Args:
        file_path (str): Target Waf script file path
    """
    # Check if file exists
    if not os.path.exists(file_path):
        print(f"Error: file {file_path} not found！", file=sys.stderr)
        sys.exit(1)
    
    # Read file content
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    # Track whether content was modified
    modified = False
    new_lines = []
    
    for line in lines:
        # Only process lines containing source=[] or includes=[] lines
        if 'source=[' in line or 'includes=[' in line:
            # Replace all 'src/' with 'source/' in the line (avoid replacing unrelated 'src')
            new_line = line.replace('src/memory-main', '../../code')
            if new_line != line:
                modified = True
                new_lines.append(new_line)
        else:
            new_lines.append(line)
    
    # 若有修改则Writefile
    if modified:
        # Backup original file (optional, prevents replacement errors)
        # backup_path = f"{file_path}.bak"
        # os.rename(file_path, backup_path)
        # print(f"already备份原file至:{backup_path}")
        
        # Write replaced content
        with open(file_path, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        # print(f"Replacedone！already将file {file_path} 中source/includesRow的'src'Replace为'source'")
    else:
        print("No 'src' content to replace found, file unchanged")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 mk_to_rtems_static.py <input.mk> [output.wscript]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    convert_mk_to_wscript(input_file, output_file)
    path = os.path.join(os.getcwd(), output_file)
    replace_src_with_source(path)