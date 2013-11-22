#include <X11/X.h>
#include <sys/time.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glx.h>

#define XRES 1280
#define YRES 720

//----------------------------------------------------------------

static const char *vsh = \
    "varying vec4 q;"
    "uniform vec2 iResolution;"

    "void main(void)"
    "{"
        "gl_Position = gl_Vertex;"
        "q = vec4(gl_Vertex.xy,gl_MultiTexCoord0.xy);"
    "}";

static const char *fsh = \
    "varying vec4 q;"
    "uniform vec2 iResolution;"

    "float sdPlane(vec3 p)"
    "{"
        "return p.y;"
    "}"

    "float sdSphere( vec3 p, float s )"
    "{"
        "return length(p)-s;"
    "}"

    "float sdBox( vec3 p, vec3 b )"
    "{"
      "vec3 d = abs(p) - b;"
      "return min(max(d.x,max(d.y,d.z)),0.0) +"
             "length(max(d,0.0));"
    "}"

    "float udRoundBox( vec3 p, vec3 b, float r )"
    "{"
      "return length(max(abs(p)-b,0.0))-r;"
    "}"

    "float sdTorus( vec3 p, vec2 t )"
    "{"
      "vec2 q = vec2(length(p.xz)-t.x,p.y);"
      "return length(q)-t.y;"
    "}"

    "float sdHexPrism( vec3 p, vec2 h )"
    "{"
        "vec3 q = abs(p);"
        "return max(q.z-h.y,max(q.x+q.y*0.57735,q.y*1.1547)-h.x);"
    "}"

    "float sdCapsule( vec3 p, vec3 a, vec3 b, float r )"
    "{"
        "vec3 pa = p - a;"
        "vec3 ba = b - a;"
        "float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );"
        
        "return length( pa - ba*h ) - r;"
    "}"

    "float sdTriPrism( vec3 p, vec2 h )"
    "{"
        "vec3 q = abs(p);"
        "return max(q.z-h.y,max(q.x*0.866025+p.y*0.5,-p.y)-h.x*0.5);"
    "}"

    "float sdCylinder( vec3 p, vec2 h )"
    "{"
        "return max( length(p.xz)-h.x, abs(p.y)-h.y );"
    "}"

    "float sdCone( in vec3 p, in vec3 c )"
    "{"
        "vec2 q = vec2( length(p.xz), p.y );"
        "return max( max( dot(q,c.xy), p.y), -p.y-c.z );"
    "}"

    "float length2( vec2 p )"
    "{"
        "return sqrt( p.x*p.x + p.y*p.y );"
    "}"

    "float length6( vec2 p )"
    "{"
        "p = p*p*p; p = p*p;"
        "return pow( p.x + p.y, 1.0/6.0 );"
    "}"

    "float length8( vec2 p )"
    "{"
        "p = p*p; p = p*p; p = p*p;"
        "return pow( p.x + p.y, 1.0/8.0 );"
    "}"

    "float sdTorus82( vec3 p, vec2 t )"
    "{"
      "vec2 q = vec2(length2(p.xz)-t.x,p.y);"
      "return length8(q)-t.y;"
    "}"

    "float sdTorus88( vec3 p, vec2 t )"
    "{"
        "vec2 q = vec2(length8(p.xz)-t.x,p.y);"
       "return length8(q)-t.y;"
    "}"

    "float sdCylinder6( vec3 p, vec2 h )"
    "{"
      "return max( length6(p.xz)-h.x, abs(p.y)-h.y );"
    "}"

    "float sdWobbleCube(vec3 p, float s)"
    "{"
        "return max("
            "max("
                "abs(p.x) - s + sin(p.y * 38.0) * 0.05, abs(p.y) - s"
            "),"
            "abs(p.z) - s"
        ");"
    "}"

    //----------------------------------------------------------------------

    "float opS( float d1, float d2 )"
    "{"
        "return max(-d2,d1);"
    "}"

    "vec2 opU( vec2 d1, vec2 d2 )"
    "{"
        "return (d1.x<d2.x) ? d1 : d2;"
    "}"

    "vec3 opRep( vec3 p, vec3 c )"
    "{"
        "return mod(p,c)-0.5*c;"
    "}"

    "vec3 opTwist( vec3 p )"
    "{"
        "float  c = cos(10.0*p.y+10.0);"
        "float  s = sin(10.0*p.y+10.0);"
        "mat2   m = mat2(c,-s,s,c);"
        "return vec3(m*p.xz,p.y);"
    "}"
    
    //----------------------------------------------------------------------

    "vec2 map( in vec3 pos )"
    "{"
        "vec2 res = opU("
            "vec2( sdPlane(pos), 1.0),"
            "vec2( sdSphere(pos - vec3(0.0, 0.25, 0.0), 0.25 ), 46.9)"
        ");"

        "res = opU("
            "res,"
            "vec2(""sdWobbleCube(pos - vec3(-1.4, 0.25, 0.0), 0.25), 113.0)"
        ");"

        "return res;"
    "}"

    "vec2 castRay( in vec3 ro, in vec3 rd, in float maxd )"
    "{"
        "float precis = 0.001;"
        "float h = precis*2.0;"
        "float t = 0.0;"
        "float m = -1.0;"
        "for( int i = 0; i < 60; i++ )"
        "{"
            "if( abs(h) < precis || t > maxd ) continue;"
            "t += h;"
            "vec2 res = map( ro+rd*t );"
            "h = res.x;"
            "m = res.y;"
        "}"

        "if( t > maxd ) m = -1.0;"

        "return vec2( t, m );"
    "}"

    "float softshadow( in vec3 ro, in vec3 rd, in float mint, in float maxt, in float k )"
    "{"
        "float res = 1.0;"
        "float t = mint;"

        "for( int i=0; i<30; i++ )"
        "{"
            "if( t<maxt )"
            "{"
                "float h = map( ro + rd*t ).x;"
                "res = min( res, k*h/t );"
                "t += 0.02;"
            "}"
        "}"

        "return clamp( res, 0.0, 1.0 );"
    "}"

    "vec3 calcNormal( in vec3 pos )"
    "{"
        "vec3 eps = vec3( 0.001, 0.0, 0.0 );"
        "vec3 nor = vec3("
            "map(pos+eps.xyy).x - map(pos-eps.xyy).x,"
            "map(pos+eps.yxy).x - map(pos-eps.yxy).x,"
            "map(pos+eps.yyx).x - map(pos-eps.yyx).x );"

        "return normalize(nor);"
    "}"

    "float calcAO( in vec3 pos, in vec3 nor )"
    "{"
        "float totao = 0.0;"
        "float sca = 1.0;"

        "for( int aoi=0; aoi<5; aoi++ )"
        "{"
            "float hr = 0.01 + 0.05*float(aoi);"
            "vec3 aopos =  nor * hr + pos;"
            "float dd = map( aopos ).x;"
            "totao += -(dd-hr)*sca;"
            "sca *= 0.75;"
        "}"

        "return clamp( 1.0 - 4.0*totao, 0.0, 1.0 );"
    "}"

    "vec3 render( in vec3 ro, in vec3 rd )"
    "{" 
        "vec3 col = vec3(0.0);"
        "vec2 res = castRay(ro,rd,20.0);"
        "float t = res.x;"
        "float m = res.y;"

        "if( m>-0.5 )"
        "{"
            "vec3 pos = ro + t*rd;"
            "vec3 nor = calcNormal( pos );"

            "col = vec3(0.6) + 0.4*sin( vec3(0.05,0.08,0.10)*(m-1.0) );"
            
            "float ao = calcAO( pos, nor );"

            "vec3 lig = normalize( vec3(-0.6, 0.7, -0.5) );"
            "float amb = clamp( 0.5+0.5*nor.y, 0.0, 1.0 );"
            "float dif = clamp( dot( nor, lig ), 0.0, 1.0 );"
            "float bac = clamp( dot( nor, normalize(vec3(-lig.x,0.0,-lig.z))), 0.0, 1.0 )*clamp( 1.0-pos.y,0.0,1.0);"

            "float sh = 1.0;"
            "if( dif>0.02 ) { sh = softshadow( pos, lig, 0.02, 10.0, 7.0 ); dif *= sh; }"

            "vec3 brdf = vec3(0.0);"
            "brdf += 0.20*amb*vec3(0.10,0.11,0.13)*ao;"
            "brdf += 0.20*bac*vec3(0.15,0.15,0.15)*ao;"
            "brdf += 1.20*dif*vec3(1.00,0.90,0.70);"

            "float pp = clamp( dot( reflect(rd,nor), lig ), 0.0, 1.0 );"
            "float spe = sh*pow(pp,16.0);"
            "float fre = ao*pow( clamp(1.0+dot(nor,rd),0.0,1.0), 2.0 );"

            "col = col*brdf + vec3(1.0)*col*spe + 0.2*fre*(0.5+0.5*col);"
           // 
        "}"
        "col *= exp( -0.01*t*t );"

        "return vec3( clamp(col,0.0,1.0) );"
    "}"

    //----------------------------------------------------------------------

    "void renderJuliaFractal(void)"
    "{"
        "vec2 z = q.xy*vec2(1.7,1.0);"
        // animate
        "vec2 c = vec2(0.5*cos(0.05*q.z) - 0.25*cos(0.1*q.z),"
        "0.5*sin(0.05*q.z) - 0.25*sin(0.1*q.z))*1.01;"
        // julia set!
        "float n;"
        "for( n=0; n<256.0 && dot(z,z)<10.0; n++ )"
        "z = vec2(z.x*z.x-z.y*z.y, 2.0*z.x*z.y) + c;"
        // smooth iterations
        "n -= log(.5*log(dot(z,z)))/log(2.0);"
        "gl_FragColor=vec4(1.0-n/256.0);"
    "}"
    
    //----------------------------------------------------------------------

    "void main(void)"
    "{"
        "vec2 resVec = gl_FragCoord.xy / iResolution.xy;"
        "vec2 p = -1.0 + 2.0 * resVec;"
        "p.x *= iResolution.x / iResolution.y;"
        //vec2 mo = iMouse.xy/iResolution.xy;
        
        "float time = q;"

        // camera	
        "vec3 ro = vec3( -0.5+3.2*cos(0.1*time + 6.0), 1.0 + 2.0, 0.5 + 3.2*sin(0.1*time + 6.0) );"
        "vec3 ta = vec3( -0.5, -0.4, 0.5 );"
        
        // camera tx
        "vec3 cw = normalize( ta-ro );"
        "vec3 cp = vec3( 0.0, 1.0, 0.0 );"
        "vec3 cu = normalize( cross(cw,cp) );"
        "vec3 cv = normalize( cross(cu,cw) );"
        "vec3 rd = normalize( p.x*cu + p.y*cv + 2.5*cw );"

        "vec3 col = render( ro, rd );"

        "col = sqrt( col );"

        "gl_FragColor=vec4( col, 1.0 );"
    "}";

//----------------------------------------------------------------

static int doubleBufferVisual[] = { GLX_RGBA,GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };

static long timeGetTime( void )
{
    struct timeval now; //, res;

    gettimeofday(&now, 0);

    return (long)((now.tv_sec*1000) + (now.tv_usec/1000));
}

//void _start()
int main( void )
{
    Display *hDisplay = XOpenDisplay( NULL );
    if( !hDisplay )return 0;

    //if( !glXQueryExtension( hDisplay, 0, 0 ) ) return 0;

    XVisualInfo *visualInfo = glXChooseVisual( hDisplay, DefaultScreen(hDisplay), doubleBufferVisual );
    if( visualInfo == NULL ) return 0;

    GLXContext hRC = glXCreateContext( hDisplay, visualInfo, NULL, GL_TRUE );
    if( hRC == NULL ) return 0;

    XSetWindowAttributes winAttr;
    winAttr.override_redirect = 1;
#if 0
    winAttr.colormap = XCreateColormap( hDisplay, RootWindow(hDisplay, visualInfo->screen),
            visualInfo->visual, AllocNone );
    Window hWnd = XCreateWindow( hDisplay, RootWindow(hDisplay, visualInfo->screen),
            0, 0, XRES, YRES, 0, visualInfo->depth, InputOutput,
            visualInfo->visual, CWColormap|CWOverrideRedirect, &winAttr );
    if( !hWnd ) return 0;
#else
    Window hWnd = XCreateSimpleWindow( hDisplay, RootWindow(hDisplay, visualInfo->screen),
            0, 0, XRES, YRES, 0, 0, 0 );
    if( !hWnd ) return 0;
    XChangeWindowAttributes(hDisplay, hWnd, CWOverrideRedirect, &winAttr);
#endif

    // move cursor out of the windows please
    XWarpPointer(hDisplay, None, hWnd, 0, 0, 0, 0, XRES, 0);

    XMapRaised(hDisplay, hWnd);

    XGrabKeyboard(hDisplay, hWnd, True, GrabModeAsync,GrabModeAsync, CurrentTime);

    glXMakeCurrent( hDisplay, hWnd, hRC );

    const int prId = glCreateProgram();
    if( !prId )return 0;
    const int vsId = glCreateShader( GL_VERTEX_SHADER );
    const int fsId = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( vsId, 1, &vsh, 0 );
    glShaderSource( fsId, 1, &fsh, 0 );
    glCompileShader( vsId );
    glCompileShader( fsId );
    glAttachShader( prId, fsId );
    glAttachShader( prId, vsId );
    glLinkProgram( prId );
    float resolution[2] = { XRES, YRES };

    long to = timeGetTime();
    XEvent event;
    while( !XCheckTypedEvent(hDisplay,KeyPress,&event ) )
    {
        glTexCoord1f( 0.001f*(timeGetTime()-to) );
        glUseProgram( prId );
        int vec2Resolution = glGetUniformLocation(prId, "iResolution");
        glUniform2fv(vec2Resolution, 1, resolution);
        glRects( -1, -1, 1, 1 );

        glXSwapBuffers( hDisplay, hWnd );
    }
    return 0;
}
