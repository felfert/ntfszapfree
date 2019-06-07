[![Build status](https://ci.appveyor.com/api/projects/status/pmo3ga8s4g9aycci?svg=true)](https://ci.appveyor.com/project/felfert/ntfszapfree)

ntfszapfree
===========

ntfszapfree is a small utility for zeroing free space on an NTFS volume.
It is intended as a simplified open-source replacement of Sysinternal's
SDelete and behaves as decribed under "How SDelete Works" (The second approach ...)
at https://docs.microsoft.com/en-us/sysinternals/downloads/sdelete
It understands only one syntax:

```zapfree -z X:```

Where X is the drive letter to be worked upon.

License
-------

This project is licensed under the MIT License.
