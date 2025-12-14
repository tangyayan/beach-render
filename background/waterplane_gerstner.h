#ifndef OCEAN_GERSTNER_FFT_H
#define OCEAN_GERSTNER_FFT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <complex>
#include <vector>
#include <cmath>
#include <random>

#include <shader.h>

#define M_PI 3.14159265358979323846

using Complex = std::complex<float>;

class OceanGerstnerFFT
{
private:
    int N;              // 网格分辨率 (必须是 2 的幂)
    float L;            // 水面边长[-L, L]
    float A;            // Phillips 谱振幅
    glm::vec2 windDir;  // 风向
    float windSpeed;    // 风速
    
    std::vector<Complex> h0;           // 初始频谱
    std::vector<Complex> h0_conj;      // 共轭频谱
    std::vector<Complex> waves;        // 波浪频谱 h_tilde(k, t)
    std::vector<Complex> waves_x;      // x 方向位移频谱
    std::vector<Complex> waves_z;      // z 方向位移频谱
    std::vector<Complex> waves_y;      // y 方向(高度)位移频谱
    
    std::vector<glm::vec3> originalPos; // 原始网格位置
    std::vector<glm::vec3> vertices;    // 最终顶点位置
    std::vector<glm::vec3> normals;     // 法线

public:
    OceanGerstnerFFT(int N = 256, float L = 1000.0f, float A = 0.0005f, 
                     glm::vec2 windDir = glm::vec2(1.0f, 1.0f), float windSpeed = 30.0f)
        : N(N), L(L), A(A), windDir(glm::normalize(windDir)), windSpeed(windSpeed)
    {
        h0.resize(N * N);
        h0_conj.resize(N * N);
        waves.resize(N * N);
        waves_x.resize(N * N);
        waves_z.resize(N * N);
        waves_y.resize(N * N);
        originalPos.resize(N * N);
        vertices.resize(N * N);
        normals.resize(N * N);
        
        InitializeSpectrum();
        InitializeOriginalPositions();
    }

    ~OceanGerstnerFFT()
    {

    }

    // Phillips 频谱
    float Phillips(glm::vec2 K)
    {
        float k_length = glm::length(K);
        if (k_length < 0.000001f) return 0.0f;
        
        float k_length2 = k_length * k_length;
        float k_length4 = k_length2 * k_length2;
        
        float k_dot_w = glm::dot(glm::normalize(K), windDir);
        float k_dot_w2 = k_dot_w * k_dot_w;
        
        float L_ = windSpeed * windSpeed / 9.81f;
        float L_2 = L_ * L_;
        
        float damping = 0.001f;
        float l2 = L_2 * damping * damping;
        
        return A * std::exp(-1.0f / (k_length2 * L_2)) / k_length4 
               * k_dot_w2 * std::exp(-k_length2 * l2);
    }

    // 初始化频谱
    void InitializeSpectrum()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int m = 0; m < N; m++) {
            for (int n = 0; n < N; n++) {
                int index = m * N + n;
                
                glm::vec2 K;
                // K.x = (2.0f * M_PI * n) / L - M_PI;
                // K.y = (2.0f * M_PI * m) / L - M_PI;
                K.x = (M_PI * (n - N / 2.0f)) / L;
                K.y = (M_PI * (m - N / 2.0f)) / L;
                
                float Ph = Phillips(K);
                
                float xi_r = dist(gen);
                float xi_i = dist(gen);
                
                h0[index] = Complex(xi_r, xi_i) * std::sqrt(Ph / 2.0f);
                h0_conj[index] = std::conj(h0[index]);
            }
        }
    }

    // 初始化原始网格位置
    void InitializeOriginalPositions()
    {
        for (int m = 0; m < N; m++) {
            for (int n = 0; n < N; n++) {
                int index = m * N + n;
                
                float x = (n / (float)(N - 1)) * 2.0f * L - L;
                // float z = m / float(N-1) * Lz;
                float z = (m / (float)(N - 1)) * 2.0f * L - L;
                //z (0到L) x (-L到L)
                
                originalPos[index] = glm::vec3(x, 0.0f, z);
            }
        }
    }

    // 核心方法: 计算 Gerstner 波的位移
    void EvaluateGerstnerWaves(float time)
    {
        // 第一步: 计算 waves[i] = h_tilde(k, t)
        for (int m = 0; m < N; m++) {
            for (int n = 0; n < N; n++) {
                int index = m * N + n;
                
                glm::vec2 K;
                K.x = (M_PI * (n - N / 2.0f)) / L;
                // K.y = (2.0f * M_PI * m) / Lz;
                K.y = (M_PI * (m - N / 2.0f)) / L;
                
                float k_length = glm::length(K);
                if (k_length < 0.0001f) {
                    waves[index] = Complex(0.0f, 0.0f);
                    continue;
                }
                
                // omega = sqrt(g * k)
                float omega = std::sqrt(9.81f * k_length);
                
                // phase = omega * time
                float phase = omega * time;
                Complex phase_complex(std::cos(phase), std::sin(phase));
                
                int conj_index = ((N - m) % N) * N + ((N - n) % N);
                
                waves[index] = h0[index] * phase_complex 
                             + std::conj(h0_conj[conj_index]) * std::conj(phase_complex);
            }
        }
        
        // 第二步: 计算 x, z, y 方向的位移频谱
        for (int i = 0; i <= N / 2; i++) {
            for (int j = 0; j < N; j++) {
                int index = i * N + j;
                
                glm::vec2 K;
                // K.x = (2.0f * M_PI * j) / L - M_PI;
                // K.y = (2.0f * M_PI * i) / L - M_PI;
                K.x = (M_PI * (j - N / 2.0f)) / L;
                // K.y = (2.0f * M_PI * i) / Lz;
                K.y = (M_PI * (i - N / 2.0f)) / L;
                
                float k_length = glm::length(K);
                if (k_length < 0.0001f) {
                    waves_x[index] = Complex(0.0f, 0.0f);
                    waves_z[index] = Complex(0.0f, 0.0f);
                    waves_y[index] = Complex(0.0f, 0.0f);
                    continue;
                }
                
                Complex p = waves[index];
                
                // 水平位移 x: waves_x[i] = -i * p = complex(-p.im, p.re)
                waves_x[index] = Complex(-p.imag(), p.real()) * (K.x / k_length);
                
                // 水平位移 z: waves_z[i] = -i * p
                waves_z[index] = Complex(-p.imag(), p.real()) * (K.y / k_length);
                
                // 垂直位移 y: waves_y[i] = p
                waves_y[index] = p;
                
                // 处理共轭部分
                int j_conj = (N - i) % N;
                int k_conj = (N - j) % N;
                int conj_index = j_conj * N + k_conj;
                
                if (conj_index != index && i <= N / 2) {
                    Complex q = waves[conj_index];
                    
                    // waves_x[j] = i * q = complex(q.im, -q.re)
                    waves_x[conj_index] = Complex(q.imag(), -q.real()) * (K.x / k_length);
                    waves_z[conj_index] = Complex(q.imag(), -q.real()) * (K.y / k_length);
                    waves_y[conj_index] = q;
                }
            }
        }
        
        // 第三步: 执行 IFFT
        std::vector<Complex> water_x = waves_x;
        std::vector<Complex> water_z = waves_z;
        std::vector<Complex> water_y = waves_y;
        
        IFFT2D(water_x);
        IFFT2D(water_z);
        IFFT2D(water_y);
        
        // 第四步: 更新顶点位置
        UpdateVerticesFromDisplacement(water_x, water_z, water_y);
    }

    // 从位移更新顶点
    void UpdateVerticesFromDisplacement(const std::vector<Complex>& water_x,
                                       const std::vector<Complex>& water_z,
                                       const std::vector<Complex>& water_y)
    {
        float scale = 70.0f; // 振幅缩放因子
        for (int m = 0; m < N; m++) {
            for (int n = 0; n < N; n++) {
                int index = m * N + n;
                
                // Gerstner 波: 最终位置 = 原始位置 + 位移
                float dx = water_x[index].real();
                float dy = water_y[index].real() * scale;
                float dz = water_z[index].real();
                
                vertices[index] = originalPos[index] + glm::vec3(dx, dy, dz);
            }
        }
        
        CalculateNormals();
    }

    // 2D IFFT
    void IFFT2D(std::vector<Complex>& data)
    {
        // 对每一行做 IFFT
        for (int m = 0; m < N; m++) {
            std::vector<Complex> row(N);
            for (int n = 0; n < N; n++) {
                row[n] = data[m * N + n];
            }
            IFFT1D(row);
            for (int n = 0; n < N; n++) {
                data[m * N + n] = row[n];
            }
        }
        
        // 对每一列做 IFFT
        for (int n = 0; n < N; n++) {
            std::vector<Complex> col(N);
            for (int m = 0; m < N; m++) {
                col[m] = data[m * N + n];
            }
            IFFT1D(col);
            for (int m = 0; m < N; m++) {
                data[m * N + n] = col[m];
            }
        }

        float scale = 1.0f / (N*N);
        for (int i = 0; i < N * N; i++) {
            data[i] *= scale;
        }
    }

    // 1D IFFT (Cooley-Tukey)
    void IFFT1D(std::vector<Complex>& data)
    {
        int n = data.size();
        if (n <= 1) return;
        
        std::vector<Complex> even(n / 2), odd(n / 2);
        for (int i = 0; i < n / 2; i++) {
            even[i] = data[i * 2];
            odd[i] = data[i * 2 + 1];
        }
        
        IFFT1D(even);
        IFFT1D(odd);
        
        for (int k = 0; k < n / 2; k++) {
            float angle = 2.0f * M_PI * k / n;
            Complex t = std::polar(1.0f, angle) * odd[k];
            data[k] = even[k] + t;
            data[k + n / 2] = even[k] - t;
        }

        // float scale = 1.0f / 2.0f;
        // for (int k = 0; k < n; k++) {
        //     data[k] *= scale;
        // }
    }

    void CalculateNormals()
    {
        for (int m = 0; m < N; m++) {
            for (int n = 0; n < N; n++) {
                int index = m * N + n;
                
                int left = m * N + (n - 1 + N) % N;
                int right = m * N + (n + 1) % N;
                int down = ((m - 1 + N) % N) * N + n;
                int up = ((m + 1) % N) * N + n;
                
                glm::vec3 tangent = vertices[right] - vertices[left];
                glm::vec3 bitangent = vertices[up] - vertices[down];
                
                normals[index] = glm::normalize(glm::cross(tangent, bitangent));
            }
        }
    }

    void Update(float time)
    {
        EvaluateGerstnerWaves(time);
    }

    float GetHeight()
    {
        return 0.0f; // Placeholder
    }
    const std::vector<glm::vec3>& GetVertices() const { return vertices; }
    const std::vector<glm::vec3>& GetOriginalPositions() const { return originalPos; }
    const std::vector<glm::vec3>& GetNormals() const { return normals; }
    public:
    // 添加调试方法
    void DebugOutput(float time)
    {
        EvaluateGerstnerWaves(time);
        
        std::cout << "\n=== Debug Output (t=" << time << ") ===" << std::endl;
        
        // 检查几个关键点
        int testIndices[] = {0, N/2, N-1, N*N/2, N*N-1};
        
        for (int idx : testIndices) {
            if (idx >= N * N) continue;
            
            int m = idx / N;
            int n = idx % N;
            
            std::cout << "Index [" << m << "," << n << "]: "
                      << "orig(" << originalPos[idx].x << "," << originalPos[idx].y << "," << originalPos[idx].z << ") "
                      << "to vert(" << vertices[idx].x << "," << vertices[idx].y << "," << vertices[idx].z << ")"
                      << std::endl;
        }
        
        // 检查位移范围
        float minDy = 1e10f, maxDy = -1e10f;
        for (int i = 0; i < N * N; i++) {
            float dy = vertices[i].y - originalPos[i].y;
            minDy = std::min(minDy, dy);
            maxDy = std::max(maxDy, dy);
        }
        
        std::cout << "Displacement range: y  [" << minDy << ", " << maxDy << "]" << std::endl;
    }
};

#endif // OCEAN_GERSTNER_FFT_H