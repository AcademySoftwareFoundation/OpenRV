
//
//   Copyright 2015 The Mill
//

const float PI = 3.141592654;
const float PI2 = PI*2.0;
const float DEG_TO_RAD = PI/180.0;
const float ONE_OVERPI = 1.0/PI;
const float ONE_OVERPI2 = 1.0/PI2;

//lat long to xyz
vec3 LatToXYZ(vec3 p)
{
    float s_theta = sin(p.x);
    float c_theta = cos(p.x);
    float s_phi    = sin(p.y);
    float c_phi   = cos(p.y);
    return vec3(s_theta*c_phi,  -c_theta, s_theta*s_phi);
}

//xyz to spherical(lat long)
vec2 XYZToLat(vec3 p)
{
    vec2 sph;
    p.y = clamp(p.y, -1.0, 1.0);
    sph.x = acos(-p.y);
    sph.y = atan(p.z,p.x);

    if(sph.y < 0.0)
        sph.y = sph.y + PI2;

    return sph;
}

vec4 main (
    const in inputImage in0,
    const float hAperture,
    const float vAperture,
    const float focalLength,
    const float rotateX,
    const float rotateY)
{
    vec2 size = in0.size();
    float vAper = vAperture;
    if (vAper == 0.0)
        vAper = hAperture * (size.y/size.x);
    float aspect = vAper/hAperture;

    //find location relative to center
    vec2 p = in0.st - size*0.5;

    //convert to physical coordiantes
    p = p *vec2(hAperture*aspect, vAper)*(1.0/size.y);

    if(abs(p.x) > hAperture*0.5 || abs(p.y) > vAper*0.5)
        return vec4(0.0,0.0,0.0,0.0);

    vec3 viewDir = vec3(clamp(rotateX*DEG_TO_RAD, 0.0001, PI - 0.0001), rotateY*DEG_TO_RAD, 1.0);
    vec3 view = LatToXYZ(viewDir);
    vec3 up = normalize(vec3(0.0,1.0,0.0) - view.y*view);
    vec3 right = cross(view, up);

    view = view*focalLength + right*p.x + up*p.y;
    view = normalize(view);
    vec2 sph = XYZToLat(view);

    return in0(vec2(sph.y*size.x*ONE_OVERPI2, sph.x*size.y*ONE_OVERPI) - in0.st);
}
