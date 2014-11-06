#version 110

// uniform parameters
float zoom = 1.0;
float tilt = 45.0;


const float pi_2 = 1.5707963267949;

float Pzz = gl_ProjectionMatrix[2][2]; 
float Pwz = gl_ProjectionMatrix[3][2]; 
float nearclip = Pwz / (Pzz - 1.0);

float aspect = gl_ProjectionMatrix[0][0] / gl_ProjectionMatrix[1][1];
float xstretch = (aspect < 1.0) ? aspect : 1.0;
float ystretch = (aspect > 1.0) ? 1.0/aspect : 1.0;

float stilt = sin(radians(tilt));
float ctilt = cos(radians(tilt));

mat4 Ttilt = mat4( 1.0, 0.0,   0.0,    0.0,
		   0.0,  ctilt, -stilt, 0.0,
		   0.0,  stilt,  ctilt, 0.0,
		   0.0, 0.0,   0.0,    1.0 );

mat4 TtiltModelView = Ttilt * gl_ModelViewMatrix;

void main() {
   vec4 p = TtiltModelView * gl_Vertex;

   float r = sqrt(p.x*p.x + p.y*p.y);
   float arc = atan( r / p.z );
   float w = (p.z < -nearclip && abs(arc) < pi_2) ? pi_2*zoom : -pi_2*zoom;
   float arc_r = arc / r;
   gl_Position = vec4( -xstretch * p.x * arc_r, -ystretch * p.y * arc_r, 0.5, w );
   gl_FrontColor = gl_Color;
   gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}
