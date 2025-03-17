# check_device_daemon

- rpm 패키지 만들기
```
rpmbuild -ba ./rpmbuild/SPECS/check_device.spec
```

- rpm 패키지 설치하기
```
rpm -ivh ./rpmbuild/RPMS/x86_64/check_device-1.0-1.el8.x86_64.rpm
```

- rpm 패키지 삭제하기
```
rpm -evh check_device
```
