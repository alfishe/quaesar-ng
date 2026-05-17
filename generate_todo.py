#!/usr/bin/env python3
"""
Parse compiler warnings from build log and generate TODO.md
Groups warnings by file with status tracking
"""

import re
import sys
from collections import defaultdict

def parse_warnings(log_file):
    """Parse warnings from build log"""
    warnings = defaultdict(list)
    
    with open(log_file, 'r') as f:
        lines = f.readlines()
    
    current_file = None
    for line in lines:
        # Match warning lines like: /path/to/file.cpp:123:45: warning: message [-Wflag]
        match = re.match(r'(/.*?):(\d+):\d+:\s+warning:\s+(.*?)\s+\[(-.*?)\]$', line.strip())
        if match:
            file_path = match.group(1)
            line_num = int(match.group(2))
            message = match.group(3)
            flag = match.group(4)
            
            # Extract just the filename for grouping
            filename = file_path.split('/')[-1]
            
            warnings[filename].append({
                'file_path': file_path,
                'line': line_num,
                'message': message,
                'flag': flag,
                'status': 'PENDING'
            })
    
    return warnings

def generate_todo_md(warnings, output_file='TODO.md'):
    """Generate TODO.md markdown file"""
    
    # Sort files by warning count (descending)
    sorted_files = sorted(warnings.items(), key=lambda x: len(x[1]), reverse=True)
    
    total_warnings = sum(len(w) for w in warnings.values())
    
    with open(output_file, 'w') as f:
        f.write("# Compiler Warning Elimination TODO\n\n")
        f.write(f"**Total Warnings:** {total_warnings}\n")
        f.write(f"**Files Affected:** {len(warnings)}\n\n")
        
        f.write("## Progress Summary\n\n")
        f.write("| Status | Count |\n")
        f.write("|--------|-------|\n")
        f.write(f"| ✅ Fixed | 0 |\n")
        f.write(f"| ⏳ Pending | {total_warnings} |\n")
        f.write(f"| **Total** | **{total_warnings}** |\n\n")
        
        f.write("---\n\n")
        f.write("## Warnings by File\n\n")
        
        for filename, file_warnings in sorted_files:
            file_path = file_warnings[0]['file_path']
            count = len(file_warnings)
            
            f.write(f"### {filename} ({count} warnings)\n\n")
            f.write(f"**Path:** `{file_path}`\n\n")
            
            f.write("| Status | Line | Type | Message |\n")
            f.write("|--------|------|------|---------|\n")
            
            for w in file_warnings:
                status = w['status']
                line = w['line']
                flag = w['flag'].replace('-W', '')
                message = w['message'][:80] + '...' if len(w['message']) > 80 else w['message']
                
                f.write(f"| {status} | {line} | `{flag}` | {message} |\n")
            
            f.write("\n---\n\n")
        
        f.write("## Fix Strategy\n\n")
        f.write("1. **Unused parameters** → Add `(void)param;` casts\n")
        f.write("2. **Unused variables** → Remove or add `(void)var;` if kept for debugging\n")
        f.write("3. **Missing struct initializers** → Add missing fields or use `= {{}}` syntax\n")
        f.write("4. **Sign comparisons** → Add explicit casts\n")
        f.write("5. **Switch statements** → Add `default:` cases\n")
        f.write("6. **Format specifiers** → Fix to correct types\n")
        f.write("7. **Missing braces** → Use `= {{}}` instead of `= {{ 0 }}`\n\n")
        
        f.write("**Rule:** NO pragma suppressions - fix warnings properly with code changes!\n")

if __name__ == '__main__':
    log_file = '/tmp/latest_build.log'
    output_file = '/Volumes/TB4-4Tb/Projects/emulators/github/quaesar-ng/TODO.md'
    
    print(f"Parsing warnings from {log_file}...")
    warnings = parse_warnings(log_file)
    
    print(f"Found {sum(len(w) for w in warnings.values())} warnings in {len(warnings)} files")
    print(f"Generating {output_file}...")
    
    generate_todo_md(warnings, output_file)
    
    print(f"✅ TODO.md generated successfully!")
    print(f"\nTop 10 files by warning count:")
    
    sorted_files = sorted(warnings.items(), key=lambda x: len(x[1]), reverse=True)
    for i, (filename, file_warnings) in enumerate(sorted_files[:10], 1):
        print(f"  {i}. {filename}: {len(file_warnings)} warnings")
