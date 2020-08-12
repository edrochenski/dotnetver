Remove-Item .\bin\release\ -Recurse -Force -ErrorAction Ignore
New-Item -Type Directory .\bin\release\ -InformationAction Ignore | Out-Null
clang -std=c99 -Ofast -Weverything --output .\bin\release\dnv.exe .\source\dotnetver.c
