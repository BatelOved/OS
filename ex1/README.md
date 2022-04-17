# smash
## Overview

“smash” (small shell) behaves like a real Linux shell but supports only a limited subset of Linux shell commands.

### Built-in commands:
- chprompt
- showpid
- pwd
- cd
- jobs
- kill
- fg
- bg
- quit

### Special commands:
- Pipes and IO redirection commands (>, <, >>, <<, |, |&)
- tail
- touch
- timeout

### External commands:
- External Bash commands (e.g., ls, cat, echo)

### Handling signals:
- Ctrl+C
- Ctrl+Z
- SIG_ALRM
