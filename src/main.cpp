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


int main(int argc, const char **argv) {

	initLoggers();

	std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
		{ std::next(argv), std::next(argv, argc) },
		true, // show help if requested
		"v3"  // version string
		);

	int count = 0;
	for (auto const &arg : args) {
		std::cout << count++ << ": " << arg.first << "=" << arg.second << std::endl;
		if(arg.first=="--gui" && arg.second.asBool()) {
			Mode=GUI;
		}
	}


	//Use the default logger (stdout, multi-threaded, colored)
	spdlog::info("Hello, {}!", "World");
	getLogger()->trace("trace message: std out only");
	getLogger()->debug("debug message std out only");
	getLogger()->info("info message both");
	getLogger()->flush();

	static char global_heap_buf[512 * 1024];
	char* buffer;
	char  error_buf[128];
	wasm_module_t module;
	//wasm_module_inst_t module_inst;
	//wasm_function_inst_t func;
	//wasm_exec_env_t exec_env;
	uint32 size;//, stack_size = 8092, heap_size = 8092;
	
	RuntimeInitArgs init_args;
	memset(&init_args, 0, sizeof(RuntimeInitArgs));
	init_args.mem_alloc_type = Alloc_With_Pool;
	init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
	init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
	// Native symbols need below registration phase
	//init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
	init_args.n_native_symbols = 0;
	init_args.native_module_name = "env";
	//init_args.native_symbols = native_symbols;
	init_args.native_symbols = nullptr;
	
	if (!wasm_runtime_full_init(&init_args)) {
		getLogger()->info("Init runtime environment failed.");
		return -1;
	} else {
		getLogger()->info("init runtime successful");
		const char *file = "./wasm-apps/mC.wasm";
		buffer = bh_read_file_to_buffer(file,&size);
		getLogger()->debug("buffer pt: {}, size {}",buffer,size);

		module = wasm_runtime_load(reinterpret_cast<uint8_t *>(buffer),size, error_buf,sizeof(error_buf));

		std::ignore = module;
	}

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
