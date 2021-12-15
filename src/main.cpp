#include <functional>
#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <docopt/docopt.h>
#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <bh_read_file.h>
#include <wasm_export.h>
#include <wsinit.h>
#include <inet/tcp.h>

static constexpr auto USAGE =
  R"(GServer3.

	Usage:
		gserver3 start [--gui]
		gserver3 (--help)
		gserver3 --version


	Options:
		--version	Show version.
		--gui	gui.
		--help	HELP.

)";

enum MODE {
  CMDLINE
  , GUI
};

static MODE Mode = CMDLINE;

static std::shared_ptr<spdlog::logger> slog; 

std::shared_ptr<spdlog::logger> &getLogger() {
	return slog;
}

void initLoggers() {
	spdlog::init_thread_pool(8192, 1);

	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
	stdout_sink->set_level(spdlog::level::trace);
	

	auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("slog.txt", 1024*1024*10, 3);
	rotating_sink->set_level(spdlog::level::info);

	std::vector<spdlog::sink_ptr> sinks {stdout_sink, rotating_sink};
	slog = std::make_shared<spdlog::async_logger>("slog", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

	slog->set_pattern("[%H:%M:%S.%f] [%t:%n] [%^%l%$] %v");
	spdlog::register_logger(slog);
	spdlog::set_default_logger(slog);
	slog->set_level(spdlog::level::trace);
}

static char * build_module_path(const char *module_name) {
    const char *module_search_path = "./wasm-apps";
    size_t sz = strlen(module_search_path) + strlen("/") + strlen(module_name) + strlen(".wasm") + 1;
    char *wasm_file_name = new char[sz]; //BH_MALLOC(sz);
    if (!wasm_file_name) {
        return NULL;
    }

    snprintf(wasm_file_name, sz, "%s/%s.wasm", module_search_path, module_name);
    return wasm_file_name;
}

static bool module_reader_cb(const char *module_name, uint8 **p_buffer, uint32 *p_size) {
	char *wasm_file_path = build_module_path(module_name);
	if (!wasm_file_path) {
		return false;
	}

	printf("- bh_read_file_to_buffer %s\n", wasm_file_path);
	*p_buffer = reinterpret_cast<uint8_t *>(bh_read_file_to_buffer(wasm_file_path, p_size));
	//BH_FREE(wasm_file_path);
	delete [] wasm_file_path;
	return *p_buffer != NULL;
}

static void module_destroyer_cb(uint8 *buffer, uint32 size) {
	std::ignore = size;
	printf("- release the read file buffer\n");
	if (!buffer) {
		return;
	}
	BH_FREE(buffer);
	//delete [] buffer;
	buffer = NULL;
}

int intToStr(wasm_exec_env_t exec_env,int x, char* str, int str_len, int digit);
int get_pow(wasm_exec_env_t exec_env,int x, int y);
int32_t calculate_native(wasm_exec_env_t exec_env,int32_t n, int32_t func1, int32_t func2);

static NativeSymbol native_symbols[] = {
	{
		"intToStr",       // the name of WASM function name
		reinterpret_cast<void*>(intToStr),         // the native function pointer
		"(i*~i)i",        // the function prototype signature, avoid to use i32
		nullptr                // attachment is NULL
	},
	{
		"get_pow",         // the name of WASM function name
		reinterpret_cast<void*>(get_pow),          // the native function pointer
		"(ii)i",       // the function prototype signature, avoid to use i32
		nullptr                // attachment is NULL
	},
	{
		"calculate_native",
		reinterpret_cast<void*>(calculate_native),
		"(iii)i",
		nullptr
	}
};



static char sandbox_memory_space[10 * 1024 * 1024] = { 0 };
int main(int argc, const char **argv) {

	initLoggers();
	wss::WSInit::init(getLogger());

	std::map<std::string, docopt::value> cmdLineArgs = docopt::docopt(USAGE,
		{ std::next(argv), std::next(argv, argc) },
		true, // show help if requested
		"v3"  // version string
		);

	int count = 0;
	for (auto const &cmdLineArg : cmdLineArgs) {
		std::cout << count++ << ": " << cmdLineArg.first << "=" << cmdLineArg.second << std::endl;
		if(cmdLineArg.first=="--gui" && cmdLineArg.second.asBool()) {
			Mode=GUI;
		}
	}


	//Use the default logger (stdout, multi-threaded, colored)
	spdlog::info("Hello, {}!", "World");
	getLogger()->trace("trace message: std out only");
	getLogger()->debug("debug message std out only");
	getLogger()->info("info message both");
	getLogger()->flush();
/////
	wss::TCPSocketInterface::initialize();
  wss::PortNum port(4321);
  wss::ListenerSocket listener(std::shared_ptr<wss::TCPSocketInterface>(wss::TCPSocketInterface::createTCPSocket()));
  wss::ErrorType et = listener.listen(port,100,false);
  wss::InetAddressV4 addr("127.0.0.1");
	if(et.ok()) {
		getLogger()->debug("listener socket good");
	}


//////////////////////////////////////

	/* 16K */
	const uint32 stack_size = 16 * 1024;
	const uint32 heap_size = 16 * 1024;

	RuntimeInitArgs init_args; // = { 0 };
	memset(&init_args,0,sizeof(init_args));
	char error_buf[128] = { 0 };
	/* parameters and return values */
	uint32_t WasmArgs[1] = { 0 };

	uint8 *file_buf = NULL;
	uint32 file_buf_size = 0;
	wasm_module_t module = NULL;
	wasm_module_inst_t module_inst = NULL;

	/* all malloc() only from the given buffer */
	init_args.mem_alloc_type = Alloc_With_Pool;
	init_args.mem_alloc_option.pool.heap_buf = sandbox_memory_space;
	init_args.mem_alloc_option.pool.heap_size = sizeof(sandbox_memory_space);
	// Native symbols need below registration phase
	init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
	init_args.native_module_name = "env";
	init_args.native_symbols = native_symbols;

	getLogger()->info("- wasm_runtime_full_init");
	/* initialize runtime environment */
	if (wasm_runtime_full_init(&init_args)) {
		getLogger()->debug("- wasm_runtime_set_module_reader\n");
		wasm_runtime_set_module_reader(module_reader_cb, module_destroyer_cb);
    	/* load WASM byte buffer from WASM bin file */
		if (module_reader_cb("mC", &file_buf, &file_buf_size)) {
			/* load mC and let WAMR load mA and mB */
			getLogger()->debug("- wasm_runtime_load\n");
			if ((module = wasm_runtime_load(file_buf, file_buf_size, error_buf, sizeof(error_buf)))) {
				/* instantiate the module */
				getLogger()->debug("- wasm_runtime_instantiate\n");
				if ((module_inst = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf)))) {
					wasm_exec_env_t exec_env = nullptr;
					if((exec_env = wasm_runtime_create_exec_env(module_inst, stack_size))!=nullptr) {

						wasm_function_inst_t funcC = wasm_runtime_lookup_function(module_inst, "C", NULL);
						wasm_function_inst_t funcCallB = wasm_runtime_lookup_function(module_inst, "call_B", NULL);
						wasm_function_inst_t funcCallA = wasm_runtime_lookup_function(module_inst, "call_A", NULL);
						wasm_function_inst_t funcmBB = wasm_runtime_lookup_function(module_inst, "$mB$B", NULL);
						wasm_function_inst_t funcmBcallA = wasm_runtime_lookup_function(module_inst, "$mB$call_A", NULL);
						wasm_function_inst_t funcmAA = wasm_runtime_lookup_function(module_inst, "$mA$A", NULL);
						wasm_function_inst_t funcGenFloat = wasm_runtime_lookup_function(module_inst, "generate_float", NULL);
						wasm_function_inst_t funcCalc = wasm_runtime_lookup_function(module_inst, "calculate", NULL);
						wasm_function_inst_t funcGetInc = wasm_runtime_lookup_function(module_inst, "getInc", NULL);
						/* call some functions of mC */
						int32_t retValf=0;
						getLogger()->info("\n----------------------------------------\n");
						getLogger()->info("call \"getInc\",  ===> ");
						assert(wasm_runtime_call_wasm(exec_env, funcGetInc, 0, WasmArgs));
						memcpy(&retValf, WasmArgs, sizeof(int32_t));
						getLogger()->info("call getInc returned: {}",retValf);
						
            getLogger()->info("\n----------------------------------------\n");
						getLogger()->info("call \"getInc\", second time ===> ");
						assert(wasm_runtime_call_wasm(exec_env, funcGetInc, 0, WasmArgs));
						memcpy(&retValf, WasmArgs, sizeof(int32_t));
						getLogger()->info("call getInc returned: {}",retValf);
            {
					    wasm_exec_env_t exec_env2 = nullptr;
					    if((exec_env2 = wasm_runtime_create_exec_env(module_inst, stack_size))!=nullptr) {
						    getLogger()->info("call getInc from new exec_env from same module instance");
						    wasm_function_inst_t funcGetInc2 = wasm_runtime_lookup_function(module_inst, "getInc", NULL);
						    assert(wasm_runtime_call_wasm(exec_env2, funcGetInc2, 0, WasmArgs));
						    memcpy(&retValf, WasmArgs, sizeof(int32_t));
						    getLogger()->info("call getInc returned: {}",retValf);
              }
            }
            {
						  getLogger()->info("create new module inst and a new exec env");
	            wasm_module_inst_t module_inst2 = NULL;
				      if ((module_inst2 = wasm_runtime_instantiate(module, stack_size, heap_size, error_buf, sizeof(error_buf)))) {
					      wasm_exec_env_t exec_env2 = nullptr;
					      if((exec_env2 = wasm_runtime_create_exec_env(module_inst2, stack_size))!=nullptr) {
						      wasm_function_inst_t funcGetInc2 = wasm_runtime_lookup_function(module_inst2, "getInc", NULL);
						      assert(wasm_runtime_call_wasm(exec_env2, funcGetInc2, 0, WasmArgs));
						      memcpy(&retValf, WasmArgs, sizeof(int32_t));
						      getLogger()->info("call getInc returned: {}",retValf);
                }
              }
            }
            getLogger()->info("\n----------------------------------------\n");
						getLogger()->info("call \"getInc\", 3rd time ===> ");
						assert(wasm_runtime_call_wasm(exec_env, funcGetInc, 0, WasmArgs));
						memcpy(&retValf, WasmArgs, sizeof(int32_t));
						getLogger()->info("call getInc returned: {}",retValf);
            sleep(1);

						getLogger()->info("\n----------------------------------------\n");
						getLogger()->info("call \"C\", it will return 0xc:i32, ===> ");
						assert(wasm_runtime_call_wasm(exec_env, funcC, 0, WasmArgs));
            sleep(1);
						memcpy(&retValf, WasmArgs, sizeof(int32_t));
						getLogger()->info("call C returned: {}",retValf);

						getLogger()->info("call \"call_B\", it will return 0xb:i32, ===> ");
						assert(wasm_runtime_call_wasm(exec_env, funcCallB, 0, WasmArgs));
            sleep(1);
						memcpy(&retValf, WasmArgs, sizeof(float));
						getLogger()->info("call Call-B returned: {}",retValf);
					
						getLogger()->info("call \"call_A\", it will return 0xa:i32, ===>");
						assert(wasm_runtime_call_wasm(exec_env, funcCallA, 0, WasmArgs));
						memcpy(&retValf, WasmArgs, sizeof(float));
						getLogger()->info("call Call-B returned: {}",retValf);

						/* call some functions of mB */
						getLogger()->info("call \"mB.B\", it will return 0xb:i32, ===>");
						assert(wasm_runtime_call_wasm(exec_env, funcmBB, 0, WasmArgs));
						memcpy(&retValf, WasmArgs, sizeof(float));
						getLogger()->info("call m$B$B returned: {}",retValf);

						getLogger()->info("call \"mB.call_A\", it will return 0xa:i32, ===>");
						assert(wasm_runtime_call_wasm(exec_env, funcmBcallA, 0, WasmArgs));
						memcpy(&retValf, WasmArgs, sizeof(float));
						getLogger()->info("call mB.call_A returned: {}",retValf);

						/* call some functions of mA */
						getLogger()->info("call \"mA.A\", it will return 0xa:i32, ===>");
						assert(wasm_runtime_call_wasm(exec_env, funcmAA, 0, WasmArgs));
						memcpy(&retValf, WasmArgs, sizeof(float));
						getLogger()->info("call mA.A returned: {}",retValf);
						getLogger()->info("----------------------------------------\n\n");

						uint32_t WasmArgs2[4] = {0};
						double arg_d = 0.000101;
						float arg_f = 300.002f;
						WasmArgs2[0] = 10;
						memcpy(&WasmArgs2[1], &arg_d, sizeof(arg_d));
						memcpy(&WasmArgs2[3], &arg_f, sizeof(arg_f));

						getLogger()->info("double size: {}",sizeof(arg_d));

						getLogger()->info("Call generate_float ====>");
						assert(wasm_runtime_call_wasm(exec_env, funcGenFloat, 4, WasmArgs2));
						memcpy(&retValf, WasmArgs2, sizeof(float));
						float *f = reinterpret_cast<float*>(&WasmArgs2[0]);
						getLogger()->info("call generate_float returned: {}",(*f));

						WasmArgs2[0] = 2;
						getLogger()->info("Call calculate ====>");
						assert(wasm_runtime_call_wasm(exec_env, funcCalc, 1, WasmArgs2));
						int32_t *t = reinterpret_cast<int32_t*>(&WasmArgs2[0]);
						getLogger()->info("call calculate returned: {}", (*t));

					} else {
						getLogger()->error("failed to create env");
					}
				} else {
					getLogger()->error("%s\n", error_buf);
				}
			} else {
				getLogger()->error("%s\n", error_buf);
			}
		} else {
			getLogger()->error("error reading");
		}
	} else {
		getLogger()->error("Init runtime environment failed.");
	}

	if(module_inst) {
		getLogger()->debug("- wasm_runtime_deinstantiate\n");
		wasm_runtime_deinstantiate(module_inst);
	}
	if(module) {
		getLogger()->debug("- wasm_runtime_unload\n");
		wasm_runtime_unload(module);
	}
	if(file_buf!=nullptr) {
		module_destroyer_cb(file_buf, file_buf_size);
	}
	getLogger()->debug("- wasm_runtime_destroy\n");
	wasm_runtime_destroy();

	if(GUI==Mode) { 
		sf::RenderWindow window(sf::VideoMode(640, 480), "ImGui + SFML = <3");
		window.setFramerateLimit(60);
		ImGui::SFML::Init(window);

		sf::CircleShape shape(100.f);
		shape.setFillColor(sf::Color::Green);

		sf::Clock deltaClock;
		while (window.isOpen()) {
			sf::Event event;
			while (window.pollEvent(event)) {
				ImGui::SFML::ProcessEvent(event);

				if (event.type == sf::Event::Closed) {
					window.close();
				}
			}

			ImGui::SFML::Update(window, deltaClock.restart());

			ImGui::Begin("Hello, world!");
			ImGui::Button("Look at this pretty button");
			ImGui::End();

			window.clear();
			window.draw(shape);
			ImGui::SFML::Render(window);
			window.display();
		}

		ImGui::SFML::Shutdown();
	}

	return 0;
}


    

