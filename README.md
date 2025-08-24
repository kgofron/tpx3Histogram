# TPX3 Histogram Program

A modern C++ implementation of the TPX3 Time-of-Flight histogram processor, rewritten from the original C version.

## Features

- **Modern C++17**: Uses RAII, smart pointers, STL containers, and exception handling
- **Object-Oriented Design**: Clean separation of concerns with dedicated classes
- **Thread-Safe**: Mutex-protected histogram processing
- **Memory Safe**: Automatic memory management with smart pointers
- **Error Handling**: Comprehensive exception handling and error reporting
- **Command Line Interface**: Configurable host and port parameters
- **Real-time Processing**: Processes incoming histogram data frames in real-time
- **Running Sum**: Maintains and updates a running sum histogram

## Architecture

The program is structured into several key classes:

- **`HistogramData`**: Represents histogram data with bin edges and values
- **`NetworkClient`**: Handles TCP socket communication
- **`HistogramProcessor`**: Processes frames and maintains running sum
- **`TPX3HistogramApp`**: Main application orchestrator

## Prerequisites

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libjson-c-dev build-essential
```

### CentOS/RHEL/Fedora
```bash
sudo yum install libjson-c-devel gcc-c++ make
```

## Building

```bash
# Build the program
make

# Or build with specific target
make all
```

## Usage

### Basic Usage
```bash
# Run with default settings (localhost:8451)
make run

# Or run directly
./tpx3_histogram
```

### Custom Host/Port
```bash
# Run with custom host and port
make run-custom HOST=192.168.1.100 PORT=9000

# Or run directly with command line arguments
./tpx3_histogram --host 192.168.1.100 --port 9000
```

### Command Line Options
- `--host HOST`: Server hostname/IP (default: 127.0.0.1)
- `--port PORT`: Server port (default: 8451)
- `--help`, `-h`: Show help message

## Data Format

The program expects TCP socket data in the following format:

```json
{
  "frameNumber": 59,
  "binSize": 10,
  "binWidth": 384000,
  "binOffset": 0,
  "dataSize": 40,
  "countSum": 7380
}
```

Followed by binary histogram data (32-bit integers in network byte order).

## Output

- **Console Output**: Real-time frame processing information
- **Data Files**: Running sum histogram saved to `data/tof-histogram-running-sum.txt`
- **Format**: Tab-separated values with bin edges and counts

## Makefile Targets

- `make all` - Build the program (default)
- `make clean` - Remove build artifacts
- `make install-deps` - Install dependencies (Ubuntu/Debian)
- `make install-deps-yum` - Install dependencies (CentOS/RHEL/Fedora)
- `make setup` - Create data directory
- `make run` - Build and run the program
- `make run-custom` - Run with custom host/port
- `make help` - Show help message

## Comparison with C Version

### Improvements in C++ Version

1. **Memory Safety**: Automatic memory management with smart pointers
2. **Exception Handling**: Comprehensive error handling with try-catch blocks
3. **Type Safety**: Strong typing with enums and const correctness
4. **RAII**: Resource acquisition is initialization for sockets and files
5. **STL Containers**: Use of `std::vector` instead of raw arrays
6. **Modern C++ Features**: Range-based for loops, auto, constexpr
7. **Thread Safety**: Mutex protection for shared data
8. **Cleaner API**: Well-defined class interfaces and encapsulation

### Performance

- Similar performance to C version
- Minimal overhead from C++ features
- Efficient memory allocation with STL containers
- Optimized compilation with `-O2` flag

## Troubleshooting

### Build Issues
- Ensure `libjson-c-dev` is installed
- Check C++17 compiler support (`g++ --version`)
- Verify all dependencies are properly installed

### Runtime Issues
- Check network connectivity to server
- Verify server is running on specified host/port
- Ensure `data/` directory exists and is writable

### Data Processing Issues
- Monitor console output for error messages
- Check JSON format of incoming data
- Verify binary data size matches expected bin count

## License

This project is licensed under the same terms as the original C version.