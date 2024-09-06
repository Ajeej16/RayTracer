
// TODO(ajeej): change the wrong return to 0

static char *
load_shader_source(const char *filename)
{
    std::string path = "..\\shaders\\";
    path += std::string(filename);
    
    FILE *file = fopen(path.c_str(), "r");
    
    if (file == NULL) {
        std::cout << "Failed to open file: " << path << std::endl;
        return NULL;
    }
    
    u64 size;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *data = (char *)malloc(size+1);
    fread(data, 1, size, file);
    
    data[size] = '\0';
    fclose(file);
    
    return data;
}

static i32
compile_shader(const char *source, u32 type)
{
    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    char info_log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        switch(type) {
            case GL_VERTEX_SHADER: {
                std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << info_log << std::endl;
            } break;
            
            case GL_FRAGMENT_SHADER: {
                std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << info_log << std::endl;
            } break;
            
            case GL_COMPUTE_SHADER: {
                std::cout << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << info_log << std::endl;
            } break;
            
            default: {
                std::cout << "ERROR::SHADER::UNKNOWN::COMPILATION_FAILED\n" << info_log << std::endl;
            } break;
        }
        
        return (i32)-1;
    }
    
    return shader;
}

static u32
create_shader(const char *vs, const char *fs)
{
    // build and compile the shaders
    // ------------------------------------
    // vertex shader
    int vertex_shader = compile_shader(vs, GL_VERTEX_SHADER);
    if(vertex_shader < 0) return -1;
    // fragment shader
    int fragment_shader = compile_shader(fs, GL_FRAGMENT_SHADER);
    if(fragment_shader < 0) return -1;
    
    // link shaders
    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, (u32)vertex_shader);
    glAttachShader(shader_program, (u32)fragment_shader);
    glLinkProgram(shader_program);
    // check for linking errors
    int success;
    char info_log[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << std::endl;
        return (u32)-1;
    }
    glDeleteShader((u32)vertex_shader);
    glDeleteShader((u32)fragment_shader);
    
    return shader_program;
}

static u32
create_compute_shader(const char *cs)
{
    i32 compute_shader = compile_shader(cs, GL_COMPUTE_SHADER);
    
    u32 shader_program = glCreateProgram();
    glAttachShader(shader_program, (u32)compute_shader);
    glLinkProgram(shader_program);
    
    int success;
    char info_log[512];
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << std::endl;
        return (u32)-1;
    }
    glDeleteShader((u32)compute_shader);
    
    return shader_program;
}