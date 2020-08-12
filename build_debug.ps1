Remove-Item .\bin\debug\ -Recurse -Force -ErrorAction Ignore
New-Item -Type Directory .\bin\debug\ -InformationAction Ignore | Out-Null
clang -std=c99 -Weverything --debug --output .\bin\debug\dnv.exe .\source\dotnetver.c
