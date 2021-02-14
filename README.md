# SimpleFS
you can clone sfs with
  ```
  https://github.com/DKU-EmbeddedSystem-Lab/SimpleFS.git
  ```
  
### compiling sfs format tool
  ```
  cd mkfs
  make
  ```
then, you can format /dev/name with
  ```
  ./mkfs /dev/name
  ```

### mount sfs on /dev/name
compile sfs
  ```
  cd ..
  make
  insmod sfs.ko
  ```
check if the module is on system
 ```
 lsmod | grep sfs
 ```
then, you can mount sfs on /dev/name with
  ```
  mount -t sfs /dev/name
  ```

if there's anything to contact <2reenact@naver.com> 
