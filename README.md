# Battleship, The game

Сервер для игры в морской бой. 

API:
1. /regnewgame 
Посылаем запрос на подбор противника, в ответ получаем номер для очереди (reg_id)
2. /regstatus?reg_id=123
Если подобрали соперника, то венет наш новый id для игры, иначе "wait"
3. /sendfield?player_id=123
Посылаем поле для игры в формате json, если оно валидно, то вернет json с "status": "true" и количество кораблей разной палубности, иначе "status": "false"
4. /trykill?player_id=123&x=0&y=0
Стреляем в точку (x, y). Получим в ответе Miss/Damage/Kill, все как в обычном морском бою. Если по дороге получили какую-то ошибку, то вернем ее. Стрелять можно только в свой ход. Попытка пострелять в чужой ход приведет к ответу "It's not your turn"
Когда все корабли противника будут уничтожены получим "You win". Или "You lose", в зависимости от ситуации.

## Makefile

Makefile contains typicaly useful targets for development:

* `make build-debug` - debug build of the service with all the assertions and sanitizers enabled
* `make build-release` - release build of the service with LTO
* `make test-debug` - does a `make build-debug` and runs all the tests on the result
* `make test-release` - does a `make build-release` and runs all the tests on the result
* `make service-start-debug` - builds the service in debug mode and starts it
* `make service-start-release` - builds the service in release mode and starts it
* `make` or `make all` - builds and runs all the tests in release and debug modes
* `make format` - autoformat all the C++ and Python sources
* `make clean-` - cleans the object files
* `make dist-clean` - clean all, including the CMake cached configurations
* `make install` - does a `make build-release` and run install in directory set in environment `PREFIX`
* `make install-debug` - does a `make build-debug` and runs install in directory set in environment `PREFIX`
* `make docker-COMMAND` - run `make COMMAND` in docker environment
* `make docker-build-debug` - debug build of the service with all the assertions and sanitizers enabled in docker environment
* `make docker-test-debug` - does a `make build-debug` and runs all the tests on the result in docker environment
* `make docker-start-service` - does a `make install-debug` and runs service in docker environment
* `make docker-start-service-debug` - does a `make install-debug` and runs service in docker environment

Edit `Makefile.local` to change the default configuration and build options.
