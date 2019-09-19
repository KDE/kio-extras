Perform smoke test against live server via kioclient. This attempts all
file operations kioclient can perform against a server you pass as argument
when calling the script.
This of course only makes sense for slaves which are readwritable.

Example invocation:
```
./smoke.sh sftp://me@localhost/tmp
```
