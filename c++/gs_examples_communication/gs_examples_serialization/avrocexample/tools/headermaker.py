import json 
import sys
from pathlib import Path

if len(sys.argv) < 3:
    print("error")
    sys.exit(1)
 
varname = sys.argv[1]
filename = sys.argv[2]
filepath=Path(filename).with_suffix(".h")
print(filepath)


with open(filename, "r") as f:
    content = f.read()
    jjj=json.loads(content)

jstringa=json.dumps(jjj)
jstringa=jstringa.replace("\"", "\\\"")
print(jstringa)


variable = f"const char {varname}[] = \"{jstringa}\";"
with open(filepath, "w") as w:
    w.write(variable)

print(f"Done {filepath} file")