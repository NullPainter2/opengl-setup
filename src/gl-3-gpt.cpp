#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <GL/gl.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct GpuModel {
    GLuint shaderProgram;
    GLuint VAO;
};

struct Gpu {
    // https://registry.khronos.org/EGL/api/KHR/khrplatform.h
    #if defined(_WIN64)
    typedef signed   long long int khronos_ssize_t;
    typedef unsigned long long int khronos_usize_t;
    #else
    typedef signed   long  int     khronos_ssize_t;
    typedef unsigned long  int     khronos_usize_t;
    #endif
    typedef khronos_ssize_t GLsizeiptr;
    
    typedef char GLchar;
    
    #define GL_ARRAY_BUFFER 0x8892
    #define GL_STATIC_DRAW 0x88E4
    #define GL_VERTEX_SHADER 0x8B31
    #define GL_FRAGMENT_SHADER 0x8B30

    #define gl_function(name,args,ret) typedef ret (*name##_t) args; name##_t name;
    #include "gl_function_list.template"
    #undef gl_function

    void init() {    
        #define gl_function(name,args,ret) name = (name##_t) wglGetProcAddress(#name);
        #include "gl_function_list.template"
        #undef gl_function
    }

    GpuModel upload_model(GLfloat * vertices, int vertices_count, char * vertexShaderSource, char * fragmentShaderSource) {
            
        // Vertex Array Object (VAO) and Vertex Buffer Object (VBO) setup
        GLuint VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices_count * sizeof(vertices[0]), vertices, GL_STATIC_DRAW);

        // Set up vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // shaders //////////////////////////////////////////////////////
        // Compile shaders
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);

        // Create shader program
        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);    
        //////////////////////////////////////////////////////
        
        GpuModel ret = {};
        ret.shaderProgram = shaderProgram;
        ret.VAO = VAO;
        return ret;
    }

    void draw_model(GpuModel & model) {
        glUseProgram(model.shaderProgram);
        glBindVertexArray(model.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);    
    }
};

struct WindowsOpenGLSetup {
    HDC hdc;
    HGLRC hrc;
    HWND hwnd;
    void init(int width, int height, char * title) {    
        // Create the window
        WNDCLASS wc = { 0 };
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
        wc.lpszClassName = "OpenGLWindow";
        RegisterClass(&wc);

        hwnd = CreateWindow("OpenGLWindow", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            100, 100, width, height, 0, 0, GetModuleHandle(NULL), 0);

    
        // Get the device context
        hdc = GetDC(hwnd);

        // Set the pixel format for the device context
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            32,
            0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0,
            24,
            8,
            0,
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
        };

        int pixelFormat = ChoosePixelFormat(hdc, &pfd);
        SetPixelFormat(hdc, pixelFormat, &pfd);

        // Create an OpenGL rendering context
        hrc = wglCreateContext(hdc);
        wglMakeCurrent(hdc, hrc);    
    }
    
    void cleanup() {
        // Clean up resources
        wglMakeCurrent(hdc, NULL);
        wglDeleteContext(hrc);
        ReleaseDC(hwnd, hdc);    
    }
};    


int main() {
    WindowsOpenGLSetup gl={};
    gl.init(800, 600, "OpenGL 3 Example");
    
    // Initialize OpenGL
    Gpu gpu = {};
    gpu.init();

    // Vertex data for a colored triangle
    GLfloat vertices[] = {
        0.0f,  0.5f, 0.0f, // Vertex 1 (x, y, z)
       -0.5f, -0.5f, 0.0f, // Vertex 2 (x, y, z)
        0.5f, -0.5f, 0.0f  // Vertex 3 (x, y, z)
    };
        
    char* vertexShaderSource = R"(
		#version 330 core
        layout (location = 0) in vec3 aPos;
        out vec4 vertexColor;
        void main()
        {
           gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
           vertexColor = vec4(0.5, 0.0, 0.0, 1.0);
        }
	)";


	// language=GLSL
    char* fragmentShaderSource = R"(
		
		#version 330 core
		
		in vec3 FragPos;  // Fragment position in world coordinates
		in vec3 Normal;   // Normal vector in world coordinates
		in vec2 TexCoords; // Texture coordinates
		
		out vec4 FragColor; // Output color of the fragment
		
		// Material properties
		struct Material {
			vec3 albedo;
			float metallic;
			float roughness;
		};
		
		uniform Material material;
		
		// Lights
		struct Light {
			vec3 position;
			vec3 color;
		};
		
		uniform Light light;
		
		// Camera
		uniform vec3 viewPos;
		
		// Function to calculate the Fresnel-Schlick approximation
		float fresnelSchlick(float cosTheta, vec3 F0) {
			return mix(pow(1.0 - cosTheta, 5.0), 1.0, F0);
		}
		
		void main() {
			// Ambient lighting
			vec3 ambient = 0.03 * material.albedo;
		
			// Diffuse lighting
			vec3 norm = normalize(Normal);
			vec3 lightDir = normalize(light.position - FragPos);
			float diff = max(dot(norm, lightDir), 0.0);
			vec3 diffuse = diff * light.color;
		
			// Specular lighting
			vec3 viewDir = normalize(viewPos - FragPos);
			vec3 halfwayDir = normalize(lightDir + viewDir);
			float specular = pow(max(dot(norm, halfwayDir), 0.0), 4.0);
			vec3 specularColor = fresnelSchlick(max(dot(norm, viewDir), 0.0), material.albedo);
		
			// Cook-Torrance BRDF
			float NDF = exp(-pow(material.roughness * (1.0 - dot(norm, halfwayDir)), 2.0) / (pow(dot(norm, halfwayDir), 2.0) * pow(material.roughness, 2.0)));
			float G = min(1.0, min(2.0 * dot(norm, halfwayDir) * dot(norm, viewDir) / dot(viewDir, halfwayDir), 2.0 * dot(norm, halfwayDir) * dot(norm, lightDir) / dot(viewDir, halfwayDir)));
			vec3 F = specularColor + (1.0 - specularColor) * pow(1.0 - dot(lightDir, halfwayDir), 5.0);
		
			vec3 specularContrib = (NDF * G * F) / (4.0 * max(dot(norm, viewDir), 0.001) * max(dot(norm, lightDir), 0.001));
		
			// Final color calculation
			vec3 lighting = ambient + diffuse + specularContrib;
			FragColor = vec4(lighting * material.albedo, 1.0);
		}

		
	)";

    GpuModel triangle = gpu.upload_model(vertices, 9, vertexShaderSource, fragmentShaderSource);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Render
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        gpu.draw_model(triangle);

        SwapBuffers(gl.hdc);
    }
    
    gl.cleanup();

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}