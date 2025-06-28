add_rules("mode.debug", "mode.release")
set_languages("c++20")

add_requires("boost", {configs = {beast = true, asio = true}})
add_requires("qt6widgets")

target("websocket_qt_viewer")
    set_kind("binary")
    add_rules("qt.widgetapp")          -- ensure Qt support for widget apps
    add_packages("boost", "qt6widgets")
    add_files("main.cpp")
