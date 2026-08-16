#!/usr/bin/env python3
import argparse, hashlib
from pathlib import Path

def main():
    parser=argparse.ArgumentParser();parser.add_argument('root',type=Path);args=parser.parse_args()
    target=args.root/'complete_bundle_checksums.sha256'
    files=sorted(p for p in args.root.rglob('*') if p.is_file() and p!=target)
    target.write_text(''.join(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(args.root)}\n" for p in files))
    print(f"checksummed {len(files)} files")
if __name__=='__main__':main()
