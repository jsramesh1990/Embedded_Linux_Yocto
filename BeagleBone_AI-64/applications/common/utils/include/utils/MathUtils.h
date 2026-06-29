#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>

namespace common {
namespace utils {

/**
 * @brief Mathematical utilities
 */
class MathUtils {
public:
    /**
     * @brief Constants
     */
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double E = 2.71828182845904523536;
    static constexpr double GOLDEN_RATIO = 1.61803398874989484820;
    
    /**
     * @brief Statistics
     */
    struct Statistics {
        double mean;
        double median;
        double variance;
        double stdDeviation;
        double min;
        double max;
        double sum;
        size_t count;
        double skewness;
        double kurtosis;
    };
    
    /**
     * @brief Linear regression result
     */
    struct LinearRegression {
        double slope;
        double intercept;
        double rSquared;
        double correlation;
        double standardError;
    };
    
    /**
     * @brief Basic math operations
     */
    template<typename T>
    static T clamp(T value, T min, T max) {
        return std::max(min, std::min(value, max));
    }
    
    template<typename T>
    static T lerp(T a, T b, double t) {
        return a + (b - a) * t;
    }
    
    template<typename T>
    static T map(T value, T inMin, T inMax, T outMin, T outMax) {
        return outMin + (outMax - outMin) * ((value - inMin) / (inMax - inMin));
    }
    
    /**
     * @brief Round to nearest multiple
     */
    static double roundToMultiple(double value, double multiple);
    
    /**
     * @brief Linear interpolation
     */
    static double lerp(double a, double b, double t);
    
    /**
     * @brief Smooth interpolation (Hermite)
     */
    static double smoothLerp(double a, double b, double t);
    
    /**
     * @brief Nearest neighbor interpolation
     */
    static double nearestLerp(double a, double b, double t);
    
    /**
     * @brief Calculate factorial
     */
    static uint64_t factorial(int n);
    
    /**
     * @brief Calculate combinations (n choose k)
     */
    static uint64_t combinations(int n, int k);
    
    /**
     * @brief Calculate permutations (nPk)
     */
    static uint64_t permutations(int n, int k);
    
    /**
     * @brief Calculate greatest common divisor
     */
    static int gcd(int a, int b);
    static int64_t gcd(int64_t a, int64_t b);
    
    /**
     * @brief Calculate least common multiple
     */
    static int lcm(int a, int b);
    static int64_t lcm(int64_t a, int64_t b);
    
    /**
     * @brief Check if number is prime
     */
    static bool isPrime(int n);
    
    /**
     * @brief Get next prime number
     */
    static int nextPrime(int n);
    
    /**
     * @brief Get prime factors
     */
    static std::vector<int> primeFactors(int n);
    
    /**
     * @brief Statistics functions
     */
    template<typename T>
    static double mean(const std::vector<T>& data);
    
    template<typename T>
    static double median(std::vector<T> data);
    
    template<typename T>
    static double variance(const std::vector<T>& data, bool sample = false);
    
    template<typename T>
    static double stdDeviation(const std::vector<T>& data, bool sample = false);
    
    template<typename T>
    static Statistics statistics(const std::vector<T>& data);
    
    /**
     * @brief Correlation
     */
    template<typename T>
    static double correlation(const std::vector<T>& x, const std::vector<T>& y);
    
    /**
     * @brief Linear regression
     */
    template<typename T>
    static LinearRegression linearRegression(const std::vector<T>& x, 
                                            const std::vector<T>& y);
    
    /**
     * @brief Normalize data (z-score)
     */
    template<typename T>
    static std::vector<double> normalizeZScore(const std::vector<T>& data);
    
    /**
     * @brief Normalize data to [0,1]
     */
    template<typename T>
    static std::vector<double> normalizeMinMax(const std::vector<T>& data);
    
    /**
     * @brief Moving average
     */
    template<typename T>
    static std::vector<double> movingAverage(const std::vector<T>& data, int window);
    
    /**
     * @brief Exponential moving average
     */
    template<typename T>
    static std::vector<double> exponentialMovingAverage(const std::vector<T>& data, double alpha);
    
    /**
     * @brief Standard deviation filter (remove outliers)
     */
    template<typename T>
    static std::vector<T> removeOutliers(const std::vector<T>& data, double sigma = 3.0);
    
    /**
     * @brief Interpolate missing values
     */
    template<typename T>
    static std::vector<T> interpolateMissing(const std::vector<T>& data, 
                                            const std::vector<bool>& missing);
    
    /**
     * @brief Geometric functions
     */
    static double distance(double x1, double y1, double x2, double y2);
    static double distance3D(double x1, double y1, double z1, double x2, double y2, double z2);
    static double angle(double x1, double y1, double x2, double y2);
    static double angleDifference(double angle1, double angle2);
    static double normalizeAngle(double angle);
    
    /**
     * @brief Convert degrees to radians
     */
    static double degToRad(double deg);
    
    /**
     * @brief Convert radians to degrees
     */
    static double radToDeg(double rad);
    
    /**
     * @brief Calculate circle circumference
     */
    static double circumference(double radius);
    
    /**
     * @brief Calculate circle area
     */
    static double circleArea(double radius);
    
    /**
     * @brief Calculate sphere volume
     */
    static double sphereVolume(double radius);
    
    /**
     * @brief Calculate distance between points (Euclidean)
     */
    template<typename T>
    static double euclideanDistance(const std::vector<T>& p1, const std::vector<T>& p2);
    
    /**
     * @brief Calculate Manhattan distance
     */
    template<typename T>
    static double manhattanDistance(const std::vector<T>& p1, const std::vector<T>& p2);
    
    /**
     * @brief Calculate Chebyshev distance
     */
    template<typename T>
    static double chebyshevDistance(const std::vector<T>& p1, const std::vector<T>& p2);
    
    /**
     * @brief Calculate cosine similarity
     */
    template<typename T>
    static double cosineSimilarity(const std::vector<T>& v1, const std::vector<T>& v2);

private:
    static std::random_device rd;
    static std::mt19937 gen;
};

} // namespace utils
} // namespace common

#endif // MATH_UTILS_H
