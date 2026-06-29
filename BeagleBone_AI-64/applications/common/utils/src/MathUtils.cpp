#include "utils/MathUtils.h"
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cassert>

using namespace common::utils;

std::random_device MathUtils::rd;
std::mt19937 MathUtils::gen(rd());

double MathUtils::roundToMultiple(double value, double multiple) {
    if (multiple == 0) return value;
    return std::round(value / multiple) * multiple;
}

double MathUtils::lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

double MathUtils::smoothLerp(double a, double b, double t) {
    t = t * t * (3 - 2 * t); // Hermite interpolation
    return a + (b - a) * t;
}

double MathUtils::nearestLerp(double a, double b, double t) {
    return t < 0.5 ? a : b;
}

uint64_t MathUtils::factorial(int n) {
    if (n < 0) return 0;
    uint64_t result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

uint64_t MathUtils::combinations(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k > n - k) k = n - k;
    uint64_t result = 1;
    for (int i = 1; i <= k; ++i) {
        result = result * (n - k + i) / i;
    }
    return result;
}

uint64_t MathUtils::permutations(int n, int k) {
    if (k < 0 || k > n) return 0;
    uint64_t result = 1;
    for (int i = n - k + 1; i <= n; ++i) {
        result *= i;
    }
    return result;
}

int MathUtils::gcd(int a, int b) {
    a = std::abs(a);
    b = std::abs(b);
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

int64_t MathUtils::gcd(int64_t a, int64_t b) {
    a = std::llabs(a);
    b = std::llabs(b);
    while (b != 0) {
        int64_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

int MathUtils::lcm(int a, int b) {
    return std::abs(a) / gcd(a, b) * std::abs(b);
}

int64_t MathUtils::lcm(int64_t a, int64_t b) {
    return std::llabs(a) / gcd(a, b) * std::llabs(b);
}

bool MathUtils::isPrime(int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    
    int limit = static_cast<int>(std::sqrt(n));
    for (int i = 3; i <= limit; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

int MathUtils::nextPrime(int n) {
    if (n < 2) return 2;
    while (true) {
        if (isPrime(n)) return n;
        ++n;
    }
}

std::vector<int> MathUtils::primeFactors(int n) {
    std::vector<int> factors;
    while (n % 2 == 0) {
        factors.push_back(2);
        n /= 2;
    }
    
    for (int i = 3; i <= std::sqrt(n); i += 2) {
        while (n % i == 0) {
            factors.push_back(i);
            n /= i;
        }
    }
    
    if (n > 2) {
        factors.push_back(n);
    }
    return factors;
}

template<typename T>
double MathUtils::mean(const std::vector<T>& data) {
    if (data.empty()) return 0.0;
    return static_cast<double>(std::accumulate(data.begin(), data.end(), 0.0)) / data.size();
}

template<typename T>
double MathUtils::median(std::vector<T> data) {
    if (data.empty()) return 0.0;
    std::sort(data.begin(), data.end());
    size_t n = data.size();
    if (n % 2 == 1) {
        return static_cast<double>(data[n / 2]);
    } else {
        return static_cast<double>(data[n / 2 - 1] + data[n / 2]) / 2.0;
    }
}

template<typename T>
double MathUtils::variance(const std::vector<T>& data, bool sample) {
    if (data.size() < 2) return 0.0;
    double meanVal = mean(data);
    double sum = 0.0;
    for (const auto& val : data) {
        double diff = static_cast<double>(val) - meanVal;
        sum += diff * diff;
    }
    return sum / (data.size() - (sample ? 1 : 0));
}

template<typename T>
double MathUtils::stdDeviation(const std::vector<T>& data, bool sample) {
    return std::sqrt(variance(data, sample));
}

template<typename T>
MathUtils::Statistics MathUtils::statistics(const std::vector<T>& data) {
    Statistics stats;
    stats.count = data.size();
    
    if (data.empty()) {
        stats.mean = stats.median = stats.min = stats.max = stats.sum = 0.0;
        stats.variance = stats.stdDeviation = stats.skewness = stats.kurtosis = 0.0;
        return stats;
    }
    
    stats.sum = std::accumulate(data.begin(), data.end(), 0.0);
    stats.mean = stats.sum / data.size();
    
    std::vector<T> sorted = data;
    std::sort(sorted.begin(), sorted.end());
    stats.min = static_cast<double>(sorted.front());
    stats.max = static_cast<double>(sorted.back());
    stats.median = median(data);
    stats.variance = variance(data, true);
    stats.stdDeviation = stdDeviation(data, true);
    
    // Skewness and kurtosis
    double m2 = 0.0, m3 = 0.0, m4 = 0.0;
    for (const auto& val : data) {
        double diff = static_cast<double>(val) - stats.mean;
        double diff2 = diff * diff;
        m2 += diff2;
        m3 += diff2 * diff;
        m4 += diff2 * diff2;
    }
    double n = static_cast<double>(data.size());
    m2 /= n;
    m3 /= n;
    m4 /= n;
    
    if (m2 > 0) {
        stats.skewness = m3 / std::pow(m2, 1.5);
        stats.kurtosis = m4 / (m2 * m2) - 3.0;
    } else {
        stats.skewness = 0.0;
        stats.kurtosis = 0.0;
    }
    
    return stats;
}

template<typename T>
double MathUtils::correlation(const std::vector<T>& x, const std::vector<T>& y) {
    if (x.size() != y.size() || x.empty()) return 0.0;
    
    double meanX = mean(x);
    double meanY = mean(y);
    
    double sumXY = 0.0, sumX2 = 0.0, sumY2 = 0.0;
    for (size_t i = 0; i < x.size(); ++i) {
        double dx = static_cast<double>(x[i]) - meanX;
        double dy = static_cast<double>(y[i]) - meanY;
        sumXY += dx * dy;
        sumX2 += dx * dx;
        sumY2 += dy * dy;
    }
    
    if (sumX2 == 0.0 || sumY2 == 0.0) return 0.0;
    return sumXY / std::sqrt(sumX2 * sumY2);
}

template<typename T>
MathUtils::LinearRegression MathUtils::linearRegression(const std::vector<T>& x, const std::vector<T>& y) {
    LinearRegression result;
    size_t n = x.size();
    
    if (n < 2) {
        result.slope = result.intercept = result.rSquared = result.correlation = result.standardError = 0.0;
        return result;
    }
    
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double xi = static_cast<double>(x[i]);
        double yi = static_cast<double>(y[i]);
        sumX += xi;
        sumY += yi;
        sumXY += xi * yi;
        sumX2 += xi * xi;
    }
    
    double meanX = sumX / n;
    double meanY = sumY / n;
    
    result.slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    result.intercept = meanY - result.slope * meanX;
    
    // R-squared
    double ssTotal = 0.0, ssResidual = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double yi = static_cast<double>(y[i]);
        double yPred = result.slope * static_cast<double>(x[i]) + result.intercept;
        ssTotal += (yi - meanY) * (yi - meanY);
        ssResidual += (yi - yPred) * (yi - yPred);
    }
    result.rSquared = 1.0 - (ssResidual / ssTotal);
    result.correlation = std::sqrt(result.rSquared) * (result.slope > 0 ? 1 : -1);
    result.standardError = std::sqrt(ssResidual / (n - 2));
    
    return result;
}

template<typename T>
std::vector<double> MathUtils::normalizeZScore(const std::vector<T>& data) {
    std::vector<double> result;
    if (data.empty()) return result;
    
    double meanVal = mean(data);
    double stdDev = stdDeviation(data);
    
    result.reserve(data.size());
    if (stdDev == 0.0) {
        for (const auto& val : data) {
            result.push_back(0.0);
        }
    } else {
        for (const auto& val : data) {
            result.push_back((static_cast<double>(val) - meanVal) / stdDev);
        }
    }
    return result;
}

template<typename T>
std::vector<double> MathUtils::normalizeMinMax(const std::vector<T>& data) {
    std::vector<double> result;
    if (data.empty()) return result;
    
    auto [minIt, maxIt] = std::minmax_element(data.begin(), data.end());
    double minVal = static_cast<double>(*minIt);
    double maxVal = static_cast<double>(*maxIt);
    
    result.reserve(data.size());
    if (maxVal == minVal) {
        for (const auto& val : data) {
            result.push_back(0.5);
        }
    } else {
        for (const auto& val : data) {
            result.push_back((static_cast<double>(val) - minVal) / (maxVal - minVal));
        }
    }
    return result;
}

template<typename T>
std::vector<double> MathUtils::movingAverage(const std::vector<T>& data, int window) {
    std::vector<double> result;
    if (data.empty() || window <= 0) return result;
    
    result.reserve(data.size());
    double sum = 0.0;
    
    for (size_t i = 0; i < data.size(); ++i) {
        sum += static_cast<double>(data[i]);
        if (i >= static_cast<size_t>(window)) {
            sum -= static_cast<double>(data[i - window]);
        }
        size_t count = std::min(i + 1, static_cast<size_t>(window));
        result.push_back(sum / count);
    }
    return result;
}

template<typename T>
std::vector<double> MathUtils::exponentialMovingAverage(const std::vector<T>& data, double alpha) {
    std::vector<double> result;
    if (data.empty() || alpha <= 0 || alpha > 1) return result;
    
    result.reserve(data.size());
    double ema = static_cast<double>(data[0]);
    result.push_back(ema);
    
    for (size_t i = 1; i < data.size(); ++i) {
        ema = alpha * static_cast<double>(data[i]) + (1 - alpha) * ema;
        result.push_back(ema);
    }
    return result;
}

template<typename T>
std::vector<T> MathUtils::removeOutliers(const std::vector<T>& data, double sigma) {
    std::vector<T> result;
    if (data.empty()) return result;
    
    double meanVal = mean(data);
    double stdDev = stdDeviation(data);
    
    result.reserve(data.size());
    for (const auto& val : data) {
        double diff = std::abs(static_cast<double>(val) - meanVal);
        if (diff <= sigma * stdDev) {
            result.push_back(val);
        }
    }
    return result;
}

template<typename T>
std::vector<T> MathUtils::interpolateMissing(const std::vector<T>& data, 
                                            const std::vector<bool>& missing) {
    if (data.size() != missing.size()) return data;
    
    std::vector<T> result = data;
    size_t n = data.size();
    
    for (size_t i = 0; i < n; ++i) {
        if (!missing[i]) continue;
        
        // Find previous and next valid values
        size_t prev = i;
        while (prev > 0 && missing[prev]) --prev;
        size_t next = i;
        while (next < n - 1 && missing[next]) ++next;
        
        if (prev == i && next == i) {
            // All values are missing
            return data;
        }
        
        if (prev == i) {
            // Only forward interpolation possible
            result[i] = data[next];
        } else if (next == i) {
            // Only backward interpolation possible
            result[i] = data[prev];
        } else {
            // Linear interpolation
            double ratio = static_cast<double>(i - prev) / (next - prev);
            double valPrev = static_cast<double>(data[prev]);
            double valNext = static_cast<double>(data[next]);
            result[i] = static_cast<T>(valPrev + (valNext - valPrev) * ratio);
        }
    }
    
    return result;
}

double MathUtils::distance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

double MathUtils::distance3D(double x1, double y1, double z1, double x2, double y2, double z2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double MathUtils::angle(double x1, double y1, double x2, double y2) {
    return std::atan2(y2 - y1, x2 - x1);
}

double MathUtils::angleDifference(double angle1, double angle2) {
    double diff = angle2 - angle1;
    while (diff > PI) diff -= 2 * PI;
    while (diff < -PI) diff += 2 * PI;
    return diff;
}

double MathUtils::normalizeAngle(double angle) {
    while (angle > PI) angle -= 2 * PI;
    while (angle < -PI) angle += 2 * PI;
    return angle;
}

double MathUtils::degToRad(double deg) {
    return deg * PI / 180.0;
}

double MathUtils::radToDeg(double rad) {
    return rad * 180.0 / PI;
}

double MathUtils::circumference(double radius) {
    return 2 * PI * radius;
}

double MathUtils::circleArea(double radius) {
    return PI * radius * radius;
}

double MathUtils::sphereVolume(double radius) {
    return 4.0 / 3.0 * PI * radius * radius * radius;
}

template<typename T>
double MathUtils::euclideanDistance(const std::vector<T>& p1, const std::vector<T>& p2) {
    if (p1.size() != p2.size()) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < p1.size(); ++i) {
        double diff = static_cast<double>(p1[i]) - static_cast<double>(p2[i]);
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

template<typename T>
double MathUtils::manhattanDistance(const std::vector<T>& p1, const std::vector<T>& p2) {
    if (p1.size() != p2.size()) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < p1.size(); ++i) {
        sum += std::abs(static_cast<double>(p1[i]) - static_cast<double>(p2[i]));
    }
    return sum;
}

template<typename T>
double MathUtils::chebyshevDistance(const std::vector<T>& p1, const std::vector<T>& p2) {
    if (p1.size() != p2.size()) return 0.0;
    double maxDiff = 0.0;
    for (size_t i = 0; i < p1.size(); ++i) {
        double diff = std::abs(static_cast<double>(p1[i]) - static_cast<double>(p2[i]));
        maxDiff = std::max(maxDiff, diff);
    }
    return maxDiff;
}

template<typename T>
double MathUtils::cosineSimilarity(const std::vector<T>& v1, const std::vector<T>& v2) {
    if (v1.size() != v2.size() || v1.empty()) return 0.0;
    
    double dot = 0.0, norm1 = 0.0, norm2 = 0.0;
    for (size_t i = 0; i < v1.size(); ++i) {
        double a = static_cast<double>(v1[i]);
        double b = static_cast<double>(v2[i]);
        dot += a * b;
        norm1 += a * a;
        norm2 += b * b;
    }
    
    if (norm1 == 0.0 || norm2 == 0.0) return 0.0;
    return dot / (std::sqrt(norm1) * std::sqrt(norm2));
}

// Explicit template instantiations
template double MathUtils::mean<int>(const std::vector<int>&);
template double MathUtils::mean<double>(const std::vector<double>&);
template double MathUtils::mean<float>(const std::vector<float>&);

template double MathUtils::median<int>(std::vector<int>);
template double MathUtils::median<double>(std::vector<double>);
template double MathUtils::median<float>(std::vector<float>);

template double MathUtils::variance<int>(const std::vector<int>&, bool);
template double MathUtils::variance<double>(const std::vector<double>&, bool);
template double MathUtils::variance<float>(const std::vector<float>&, bool);

template double MathUtils::stdDeviation<int>(const std::vector<int>&, bool);
template double MathUtils::stdDeviation<double>(const std::vector<double>&, bool);
template double MathUtils::stdDeviation<float>(const std::vector<float>&, bool);

template MathUtils::Statistics MathUtils::statistics<int>(const std::vector<int>&);
template MathUtils::Statistics MathUtils::statistics<double>(const std::vector<double>&);
template MathUtils::Statistics MathUtils::statistics<float>(const std::vector<float>&);

template double MathUtils::correlation<int>(const std::vector<int>&, const std::vector<int>&);
template double MathUtils::correlation<double>(const std::vector<double>&, const std::vector<double>&);

template MathUtils::LinearRegression MathUtils::linearRegression<int>(const std::vector<int>&, const std::vector<int>&);
template MathUtils::LinearRegression MathUtils::linearRegression<double>(const std::vector<double>&, const std::vector<double>&);

template std::vector<double> MathUtils::normalizeZScore<int>(const std::vector<int>&);
template std::vector<double> MathUtils::normalizeZScore<double>(const std::vector<double>&);

template std::vector<double> MathUtils::normalizeMinMax<int>(const std::vector<int>&);
template std::vector<double> MathUtils::normalizeMinMax<double>(const std::vector<double>&);

template std::vector<double> MathUtils::movingAverage<int>(const std::vector<int>&, int);
template std::vector<double> MathUtils::movingAverage<double>(const std::vector<double>&, int);

template std::vector<double> MathUtils::exponentialMovingAverage<int>(const std::vector<int>&, double);
template std::vector<double> MathUtils::exponentialMovingAverage<double>(const std::vector<double>&, double);

template std::vector<int> MathUtils::removeOutliers<int>(const std::vector<int>&, double);
template std::vector<double> MathUtils::removeOutliers<double>(const std::vector<double>&, double);

template std::vector<int> MathUtils::interpolateMissing<int>(const std::vector<int>&, const std::vector<bool>&);
template std::vector<double> MathUtils::interpolateMissing<double>(const std::vector<double>&, const std::vector<bool>&);

template double MathUtils::euclideanDistance<int>(const std::vector<int>&, const std::vector<int>&);
template double MathUtils::euclideanDistance<double>(const std::vector<double>&, const std::vector<double>&);

template double MathUtils::manhattanDistance<int>(const std::vector<int>&, const std::vector<int>&);
template double MathUtils::manhattanDistance<double>(const std::vector<double>&, const std::vector<double>&);

template double MathUtils::chebyshevDistance<int>(const std::vector<int>&, const std::vector<int>&);
template double MathUtils::chebyshevDistance<double>(const std::vector<double>&, const std::vector<double>&);

template double MathUtils::cosineSimilarity<int>(const std::vector<int>&, const std::vector<int>&);
template double MathUtils::cosineSimilarity<double>(const std::vector<double>&, const std::vector<double>&);
