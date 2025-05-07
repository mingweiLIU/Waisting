#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
};
layout(binding = 1) uniform sampler2D colorSource;
layout(binding = 2) uniform sampler2D maskSource;

bool noData(vec2 texCoord,float xRadiusRate,float yRadiusRate){
    if(radiusRate>0.5f) radiusRate=0.5f;
    if(texCoord.x<radiusRate&&texCoord.y<radiusRate){
        return length(texCoord-vec2(radiusRate,radiusRate))>radiusRate;
    }else if(texCoord.x>(1-radiusRate)&&texCoord.y<radiusRate){
        return length(texCoord-vec2(1-radiusRate,radiusRate))>radiusRate;
    }else if(texCoord.x>(1-radiusRate)&&texCoord.y>(1-radiusRate)){
        return length(texCoord-vec2(1-radiusRate,1-radiusRate))>radiusRate;
    }else if(texCoord.x<radiusRate&&texCoord.y>(1-radiusRate)){
        return length(texCoord-vec2(radiusRate,1-radiusRate))>radiusRate;
    }
    return false;
}

bool noData2(vec2 texCoord,float width,float height,float r){
    float xR=r/width;
    float yR=r/height;
    if(xR>0.5) xR=0.5;
    if(yR>0.5) yR=0.5;
}


void main() {
    vec4 orange=vec4(237, 194, 39,255.0)/255.0;
    vec4 gray=vec4(141, 141, 141.0,255)/255.0;
    vec2 center=vec2(0.5,1);
    float dis= sin(3.14*qt_TexCoord0.x)*qt_TexCoord0.y;
    vec4 temp = dis*(orange-gray)+gray;
    if(noData(qt_TexCoord0,0.1))
        discard;
    else 
        fragColor=temp;
}