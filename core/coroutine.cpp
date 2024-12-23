#include "coroutine.h"

lin_io::ThreadLocal<lin_io::Coroutine>                  lin_io::Coroutine::_running ;
lin_io::ThreadLocal<lin_io::Coroutine::MainContext>     lin_io::Coroutine::_main(lin_io::Coroutine::_release_main_context) ;
