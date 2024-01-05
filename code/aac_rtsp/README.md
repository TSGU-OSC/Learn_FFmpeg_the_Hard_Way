# Complie Only

- Generate binary executable file
```
    make
```

- Clear binary executable file
```
    make clean
```

- Compile and execute
```
    ./build_and_run.sh
```

#### After executing an executable binary file and obtaining the RTSP stream address,ffplay can be used to play directly.
- The playback command is   
```
    ffplay -i rtsp://192.168.50.236
```
- Modify IP address based on actual situation.