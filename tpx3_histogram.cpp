#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <stdexcept>
#include <system_error>
#include <cstring>
#include <iomanip>
#include <filesystem>

// Network includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// JSON parsing
#include <json-c/json.h>

// Constants
constexpr double TPX3_TDC_CLOCK_PERIOD_SEC = (1.5625 / 6.0) * 1e-9;
constexpr size_t MAX_BUFFER_SIZE = 32768;
constexpr size_t MAX_BINS = 1000;
constexpr int DEFAULT_PORT = 8451;
constexpr const char* DEFAULT_HOST = "127.0.0.1";

// Forward declarations
class HistogramData;
class NetworkClient;
class HistogramProcessor;

/**
 * @brief Represents histogram data with bin edges and values
 */
class HistogramData {
public:
    enum class DataType {
        FRAME_DATA,      // Individual frame data (32-bit)
        RUNNING_SUM      // Accumulated data (64-bit)
    };

    HistogramData(size_t bin_size, DataType type = DataType::FRAME_DATA)
        : bin_size_(bin_size), data_type_(type) {
        
        bin_edges_.resize(bin_size + 1);
        
        if (type == DataType::FRAME_DATA) {
            bin_values_32_.resize(bin_size, 0);
        } else {
            bin_values_64_.resize(bin_size, 0);
        }
    }

    // Copy constructor
    HistogramData(const HistogramData& other)
        : bin_size_(other.bin_size_), data_type_(other.data_type_) {
        
        bin_edges_ = other.bin_edges_;
        
        if (data_type_ == DataType::FRAME_DATA) {
            bin_values_32_ = other.bin_values_32_;
        } else {
            bin_values_64_ = other.bin_values_64_;
        }
    }

    // Move constructor
    HistogramData(HistogramData&& other) noexcept
        : bin_size_(other.bin_size_), data_type_(other.data_type_) {
        
        bin_edges_ = std::move(other.bin_edges_);
        bin_values_32_ = std::move(other.bin_values_32_);
        bin_values_64_ = std::move(other.bin_values_64_);
        
        other.bin_size_ = 0;
        other.data_type_ = DataType::FRAME_DATA;
    }

    // Assignment operators
    HistogramData& operator=(const HistogramData& other) {
        if (this != &other) {
            bin_size_ = other.bin_size_;
            data_type_ = other.data_type_;
            bin_edges_ = other.bin_edges_;
            
            if (data_type_ == DataType::FRAME_DATA) {
                bin_values_32_ = other.bin_values_32_;
            } else {
                bin_values_64_ = other.bin_values_64_;
            }
        }
        return *this;
    }

    HistogramData& operator=(HistogramData&& other) noexcept {
        if (this != &other) {
            bin_size_ = other.bin_size_;
            data_type_ = other.data_type_;
            bin_edges_ = std::move(other.bin_edges_);
            bin_values_32_ = std::move(other.bin_values_32_);
            bin_values_64_ = std::move(other.bin_values_64_);
            
            other.bin_size_ = 0;
            other.data_type_ = DataType::FRAME_DATA;
        }
        return *this;
    }

    // Destructor
    ~HistogramData() = default;

    // Getters
    size_t get_bin_size() const { return bin_size_; }
    DataType get_data_type() const { return data_type_; }
    const std::vector<double>& get_bin_edges() const { return bin_edges_; }
    
    // Access bin values based on type
    uint32_t get_bin_value_32(size_t index) const {
        if (data_type_ != DataType::FRAME_DATA || index >= bin_values_32_.size()) {
            throw std::out_of_range("Invalid index or data type for 32-bit access");
        }
        return bin_values_32_[index];
    }
    
    uint64_t get_bin_value_64(size_t index) const {
        if (data_type_ != DataType::RUNNING_SUM || index >= bin_values_64_.size()) {
            throw std::out_of_range("Invalid index or data type for 64-bit access");
        }
        return bin_values_64_[index];
    }

    // Setters
    void set_bin_edge(size_t index, double value) {
        if (index >= bin_edges_.size()) {
            throw std::out_of_range("Bin edge index out of range");
        }
        bin_edges_[index] = value;
    }

    void set_bin_value_32(size_t index, uint32_t value) {
        if (data_type_ != DataType::FRAME_DATA || index >= bin_values_32_.size()) {
            throw std::out_of_range("Invalid index or data type for 32-bit access");
        }
        bin_values_32_[index] = value;
    }

    void set_bin_value_64(size_t index, uint64_t value) {
        if (data_type_ != DataType::RUNNING_SUM || index >= bin_values_64_.size()) {
            throw std::out_of_range("Invalid index or data type for 64-bit access");
        }
        bin_values_64_[index] = value;
    }

    // Calculate bin edges from parameters
    void calculate_bin_edges(int bin_width, int bin_offset) {
        for (size_t i = 0; i < bin_edges_.size(); ++i) {
            bin_edges_[i] = (bin_offset + (i * bin_width)) * TPX3_TDC_CLOCK_PERIOD_SEC;
        }
    }

    // Add another histogram to this one (for running sum)
    void add_histogram(const HistogramData& other) {
        if (other.data_type_ != DataType::FRAME_DATA || data_type_ != DataType::RUNNING_SUM) {
            throw std::invalid_argument("Can only add frame data to running sum");
        }
        
        if (other.bin_size_ != bin_size_) {
            throw std::invalid_argument("Bin sizes must match for addition");
        }

        for (size_t i = 0; i < bin_size_; ++i) {
            uint64_t new_value = bin_values_64_[i] + other.bin_values_32_[i];
            if (new_value < bin_values_64_[i]) {
                std::cerr << "Warning: Overflow detected in bin " << i 
                          << ", capping at maximum value" << std::endl;
                bin_values_64_[i] = UINT64_MAX;
            } else {
                bin_values_64_[i] = new_value;
            }
        }
    }

private:
    size_t bin_size_;
    DataType data_type_;
    std::vector<double> bin_edges_;
    std::vector<uint32_t> bin_values_32_;
    std::vector<uint64_t> bin_values_64_;
};

/**
 * @brief Network client for TCP socket communication
 */
class NetworkClient {
public:
    NetworkClient() : socket_fd_(-1), connected_(false) {}
    
    ~NetworkClient() {
        disconnect();
    }

    // Disable copy
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Allow move
    NetworkClient(NetworkClient&& other) noexcept
        : socket_fd_(other.socket_fd_), connected_(other.connected_) {
        other.socket_fd_ = -1;
        other.connected_ = false;
    }

    NetworkClient& operator=(NetworkClient&& other) noexcept {
        if (this != &other) {
            disconnect();
            socket_fd_ = other.socket_fd_;
            connected_ = other.connected_;
            other.socket_fd_ = -1;
            other.connected_ = false;
        }
        return *this;
    }

    /**
     * @brief Connect to server
     * @param host Server hostname/IP
     * @param port Server port
     * @return true if successful, false otherwise
     */
    bool connect(const std::string& host, int port) {
        socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
            return false;
        }

        // Set TCP_NODELAY to disable Nagle's algorithm
        int flag = 1;
        if (setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
            std::cerr << "Failed to set TCP_NODELAY: " << strerror(errno) << std::endl;
        }

        // Set larger socket buffers
        int rcvbuf = 256 * 1024;  // 256KB receive buffer
        if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0) {
            std::cerr << "Failed to set receive buffer size: " << strerror(errno) << std::endl;
        }

        struct sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << host << std::endl;
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }

        std::cout << "Attempting to connect to " << host << ":" << port << "..." << std::endl;
        
        if (::connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed: " << strerror(errno) << std::endl;
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }
        
        std::cout << "Connected successfully" << std::endl;
        connected_ = true;
        return true;
    }

    /**
     * @brief Disconnect from server
     */
    void disconnect() {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
        connected_ = false;
    }

    /**
     * @brief Check if connected
     * @return true if connected
     */
    bool is_connected() const { return connected_; }

    /**
     * @brief Receive data from socket
     * @param buffer Buffer to store received data
     * @param max_size Maximum size to receive
     * @return Number of bytes received, -1 on error, 0 on connection closed
     */
    ssize_t receive(char* buffer, size_t max_size) {
        if (!connected_ || socket_fd_ < 0) {
            return -1;
        }
        
        ssize_t bytes_read = recv(socket_fd_, buffer, max_size, 0);
        
        if (bytes_read == 0) {
            std::cout << "Connection closed by peer" << std::endl;
            connected_ = false;
        } else if (bytes_read < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Socket error: " << strerror(errno) << std::endl;
                connected_ = false;
            }
        }
        
        return bytes_read;
    }

    /**
     * @brief Receive exact amount of data
     * @param buffer Buffer to store received data
     * @param size Exact size to receive
     * @return true if successful, false otherwise
     */
    bool receive_exact(char* buffer, size_t size) {
        size_t total_received = 0;
        
        while (total_received < size) {
            ssize_t bytes = receive(buffer + total_received, size - total_received);
            
            if (bytes <= 0) {
                return false;
            }
            
            total_received += bytes;
        }
        
        return true;
    }

private:
    int socket_fd_;
    bool connected_;
};

/**
 * @brief Processes histogram data and maintains running sum
 */
class HistogramProcessor {
public:
    HistogramProcessor() = default;
    
    ~HistogramProcessor() = default;

    // Disable copy
    HistogramProcessor(const HistogramProcessor&) = delete;
    HistogramProcessor& operator=(const HistogramProcessor&) = delete;

    // Allow move
    HistogramProcessor(HistogramProcessor&&) = default;
    HistogramProcessor& operator=(HistogramProcessor&&) = default;

    /**
     * @brief Process a new frame of histogram data
     * @param frame_data Frame histogram data
     */
    void process_frame(const HistogramData& frame_data) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!running_sum_) {
            // Initialize running sum with same bin size
            running_sum_ = std::make_unique<HistogramData>(
                frame_data.get_bin_size(), 
                HistogramData::DataType::RUNNING_SUM
            );
            
            // Copy bin edges
            for (size_t i = 0; i < frame_data.get_bin_edges().size(); ++i) {
                running_sum_->set_bin_edge(i, frame_data.get_bin_edges()[i]);
            }
        }
        
        // Add frame data to running sum
        running_sum_->add_histogram(frame_data);
        
        // Save updated running sum
        save_running_sum();
    }

    /**
     * @brief Get current running sum
     * @return Pointer to running sum histogram (nullptr if none exists)
     */
    const HistogramData* get_running_sum() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return running_sum_.get();
    }

    /**
     * @brief Save running sum to file
     */
    void save_running_sum() {
        if (!running_sum_) {
            return;
        }
        
        std::string filename = "data/tof-histogram-running-sum.txt";
        save_histogram_to_file(filename, *running_sum_);
    }

private:
    /**
     * @brief Save histogram data to file
     * @param filename Output filename
     * @param histogram Histogram data to save
     */
    void save_histogram_to_file(const std::string& filename, const HistogramData& histogram) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        // Set larger buffer for file I/O
        file.rdbuf()->pubsetbuf(nullptr, 0);  // Use default buffering

        file << "# Time of Flight Histogram Data\n";
        file << "# Bins: " << histogram.get_bin_size() << "\n";
        file << "#\n";

        for (size_t i = 0; i < histogram.get_bin_size(); ++i) {
            if (histogram.get_data_type() == HistogramData::DataType::RUNNING_SUM) {
                file << std::scientific << std::setprecision(9) 
                     << histogram.get_bin_edges()[i] << "\t" 
                     << histogram.get_bin_value_64(i) << "\n";
            } else {
                file << std::scientific << std::setprecision(9) 
                     << histogram.get_bin_edges()[i] << "\t" 
                     << histogram.get_bin_value_32(i) << "\n";
            }
        }
        
        // Write last bin edge
        file << std::scientific << std::setprecision(9) 
             << histogram.get_bin_edges()[histogram.get_bin_size()] << "\n";
        
        file.close();
    }

    mutable std::mutex mutex_;
    std::unique_ptr<HistogramData> running_sum_;
};

/**
 * @brief Main application class
 */
class TPX3HistogramApp {
public:
    TPX3HistogramApp() = default;
    
    ~TPX3HistogramApp() = default;

    // Disable copy
    TPX3HistogramApp(const TPX3HistogramApp&) = delete;
    TPX3HistogramApp& operator=(const TPX3HistogramApp&) = delete;

    /**
     * @brief Run the application
     * @param host Server hostname/IP
     * @param port Server port
     * @return Exit code
     */
    int run(const std::string& host = DEFAULT_HOST, int port = DEFAULT_PORT) {
        // Create data directory
        std::filesystem::create_directories("data");
        
        // Connect to server
        if (!client_.connect(host, port)) {
            return 1;
        }

        std::cout << "Waiting for data..." << std::endl;
        
        std::vector<char> line_buffer(MAX_BUFFER_SIZE);
        size_t total_read = 0;

        try {
            while (client_.is_connected()) {
                ssize_t bytes_read = client_.receive(
                    line_buffer.data() + total_read, 
                    MAX_BUFFER_SIZE - total_read - 1
                );

                if (bytes_read <= 0) {
                    break;
                }

                total_read += bytes_read;
                line_buffer[total_read] = '\0';
                
                // Look for newline
                char* newline_pos = static_cast<char*>(memchr(line_buffer.data(), '\n', total_read));
                if (newline_pos) {
                    *newline_pos = '\0';
                    
                    // Process complete line
                    if (!process_data_line(line_buffer.data(), newline_pos, total_read)) {
                        break;
                    }
                    
                    // Move remaining data to start of buffer
                    size_t remaining = total_read - (newline_pos - line_buffer.data() + 1);
                    if (remaining > 0) {
                        memmove(line_buffer.data(), newline_pos + 1, remaining);
                    }
                    total_read = remaining;
                }
                
                // Prevent buffer overflow
                if (total_read >= MAX_BUFFER_SIZE - 1) {
                    std::cout << "Buffer full, resetting" << std::endl;
                    total_read = 0;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

        std::cout << "\n*** Ready ***" << std::endl;
        return 0;
    }

private:
    /**
     * @brief Process a complete data line
     * @param line_buffer Buffer containing the line
     * @param newline_pos Position of newline character
     * @param total_read Total bytes read so far
     * @return true if successful, false to exit
     */
    bool process_data_line(char* line_buffer, char* newline_pos, size_t total_read) {
        // Parse JSON
        struct json_object* json = json_tokener_parse(line_buffer);
        if (!json) {
            return true;  // Continue processing
        }

        try {
            // Extract header information
            struct json_object *frame_number_obj, *bin_size_obj, *bin_width_obj, *bin_offset_obj;
            
            if (!json_object_object_get_ex(json, "frameNumber", &frame_number_obj) ||
                !json_object_object_get_ex(json, "binSize", &bin_size_obj) ||
                !json_object_object_get_ex(json, "binWidth", &bin_width_obj) ||
                !json_object_object_get_ex(json, "binOffset", &bin_offset_obj)) {
                
                json_object_put(json);
                return true;  // Continue processing
            }

            int frame_number = json_object_get_int(frame_number_obj);
            int bin_size = json_object_get_int(bin_size_obj);
            int bin_width = json_object_get_int(bin_width_obj);
            int bin_offset = json_object_get_int(bin_offset_obj);

            // Create frame histogram
            HistogramData frame_histogram(bin_size, HistogramData::DataType::FRAME_DATA);
            
            // Calculate bin edges
            frame_histogram.calculate_bin_edges(bin_width, bin_offset);

            // Read binary data
            std::vector<uint32_t> tof_bin_values(bin_size);
            size_t binary_needed = bin_size * sizeof(uint32_t);
            
            // Copy any binary data we already have after the newline
            size_t remaining = total_read - (newline_pos - line_buffer + 1);
            size_t binary_read = 0;
            
            if (remaining > 0) {
                size_t to_copy = std::min(remaining, binary_needed);
                memcpy(tof_bin_values.data(), newline_pos + 1, to_copy);
                binary_read = to_copy;
            }

            // Read any remaining binary data needed
            if (binary_read < binary_needed) {
                if (!client_.receive_exact(
                    reinterpret_cast<char*>(tof_bin_values.data()) + binary_read,
                    binary_needed - binary_read)) {
                    
                    std::cerr << "Failed to read binary data" << std::endl;
                    json_object_put(json);
                    return false;
                }
            }

            // Convert to little-endian
            for (int i = 0; i < bin_size; ++i) {
                tof_bin_values[i] = __builtin_bswap32(tof_bin_values[i]);
                frame_histogram.set_bin_value_32(i, tof_bin_values[i]);
            }

            // Print frame information
            std::cout << "\nFrame " << frame_number << " data:" << std::endl;
            std::cout << "Bin edges: ";
            for (int i = 0; i < bin_size + 1; ++i) {
                std::cout << std::scientific << std::setprecision(9) 
                          << frame_histogram.get_bin_edges()[i] << " ";
            }
            std::cout << "\nBin values: ";
            for (int i = 0; i < bin_size; ++i) {
                std::cout << frame_histogram.get_bin_value_32(i) << " ";
            }
            std::cout << "\n" << std::endl;

            // Process frame
            processor_.process_frame(frame_histogram);
            
            std::cout << "Frame " << frame_number << " processed (running sum updated)" << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error processing frame: " << e.what() << std::endl;
        }

        json_object_put(json);
        return true;
    }

    NetworkClient client_;
    HistogramProcessor processor_;
};

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    std::string host = DEFAULT_HOST;
    int port = DEFAULT_PORT;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [--host HOST] [--port PORT] [--help]\n"
                      << "  --host HOST    Server hostname/IP (default: " << DEFAULT_HOST << ")\n"
                      << "  --port PORT    Server port (default: " << DEFAULT_PORT << ")\n"
                      << "  --help, -h     Show this help message\n";
            return 0;
        }
    }

    try {
        TPX3HistogramApp app;
        return app.run(host, port);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
