#define alpha_phong 0.0
float phong_D(vec3 m, vec3 n)
{
    float mDotn = dot(m, n);
    float theta_m = acos(mDotn);
    return (mDotn > 0 ? ((alpha_phong + 2) * pow(cos(theta_m), alpha_phong)) / (2 * PI) : 0);
};

float phong_g1(vec3 v, vec3 m, vec3 n)
{
    if (dot(v, m) * dot(v, n) > 0){
        float theta_v = acos(dot(v, n));
        float a = sqrt(0.5 * alpha_phong + 1) / abs(tan(theta_v));
        float ret = a < 1.6 ? (3.535 * a + 2.181 * pow(a, 2)) / (1 + 2.276 * a + 2.577 * pow(a, 2)) : 1;
        return ret;
    } else {
        return 0;
    }
};

vec3 phong_sampling(vec3 N, float rand1, float rand2)
{
    float theta = acos(pow(rand1, 1 / (alpha_phong + 2)));
    float phi = 2 * PI * rand2;
    
	// T is perpendicular to N.
	vec3 T = cross(N, vec3(0.0,0.0,1.0));
	return rotate_point(rotate_point(N, T, theta), N, phi);
};