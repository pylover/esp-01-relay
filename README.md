
# Helloword 

### Build

Follow [this](https://github.com/pylover/esp8266-env) instruction 
to setup your environment.

```bash
cd esp8266-env
source activate.sh
cd ..

cd <project>
bash gen_misc.sh
```

Or use predefined make macros:

```bash
make clean
make flash_map6user1 

cd webui
npm install
cd ..
make flash_map6webui

```

