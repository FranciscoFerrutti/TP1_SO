# TP1_SO

Program for generating md5 hashes

Compiling instructions
```
$   make all
```

To execute the program
```
$   <path_to_md5.elf>/md5.elf <files_for_hashing>
```

For seeing in real time the hash generation, execute the following
```
$   ./<path_to_md5.elf>/md5.elf <files_for_hashing> | ./<path_to_vision.elf>/vision.elf
```

For removing the generated files

```
$   make clean
```
