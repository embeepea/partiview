
/* HSV to RGB conversion from Ken Fishkin, pixar!fishkin */
void hsb2rgb(float vH, float vS, float vB, float *rp, float *gp, float *bp)
{
    float h = 6.0 * (vH < 0 ? vH + (1 - (int)vH) : vH - (int)vH);
    int sextant = (int) h; /* implicit floor */
    float fract = h - sextant;
    float vsf = vS*vB*fract;
    float min = (1-vS)*vB;
    float mid1 = min + vsf;
    float mid2 = vB - vsf;
    switch (sextant%6) {
    case 0: *rp = vB;   *gp = mid1; *bp = min; break;
    case 1: *rp = mid2; *gp = vB;   *bp = min; break;
    case 2: *rp = min;  *gp = vB;   *bp = mid1; break;
    case 3: *rp = min;  *gp = mid2; *bp = vB; break;
    case 4: *rp = mid1; *gp = min;  *bp = vB; break;
    case 5: *rp = vB;   *gp = min;  *bp = mid2; break;
    }
}

void rgb2hsb(float r, float g, float b,  float *hp, float *sp, float *bp)
{
    float cRGB[3];
    int min, max;
    float dv;

    cRGB[0] = r, cRGB[1] = g, cRGB[2] = b;
    if(cRGB[0] < cRGB[1])
	min = 0, max = 1;
    else
	min = 1, max = 0;
    if(cRGB[min] > cRGB[2]) min = 2;
    else if(cRGB[max] < cRGB[2]) max = 2;

    *bp = cRGB[max];
    dv = cRGB[max] - cRGB[min];
    if(dv == 0) {
	*hp = 0;	/* hue undefined, use 0 */
	*sp = 0;
    } else {
	float dh = (cRGB[3 - max - min] - cRGB[min]) / (6*dv);
	*hp = (3+max-min)%3==1 ? max/3.0 + dh : max/3.0 - dh;
	if(*hp < 0) *hp += 1 + (int)*hp;
	if(*hp > 1) *hp -= (int)*hp;
	*sp = dv / cRGB[max];
    }
}
