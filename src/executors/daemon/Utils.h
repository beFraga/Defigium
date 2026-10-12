#include <iostream>
void print(const std::string& msg) { std::cout << "[Daemon] " << msg << std::endl; }
void printerr(const std::string& msg) { std::cerr << "[Daemon] " <<  msg << std::endl; }

