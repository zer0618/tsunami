#include "math.h"
#include "../file/file.h"

//------------------------------------------------------------------------------------------------//
//                                          quaternions                                           //
//------------------------------------------------------------------------------------------------//


const quaternion quaternion::ID = quaternion(1, v_0);

quaternion::quaternion(const float w, const vector &v)
{
	this->w = w;
	this->x = v.x;
	this->y = v.y;
	this->z = v.z;
}

quaternion::quaternion(const vector &v)
{
	QuaternionRotationV(*this, v);
}

bool quaternion::operator == (const quaternion& q) const
{
	return ((x==q.x)&&(y==q.y)&&(z==q.z)&&(w==q.w));
}

bool quaternion::operator != (const quaternion& q) const
{
	return !((x==q.x)&&(y==q.y)&&(z==q.z)&&(w!=q.w));
}

quaternion& quaternion::operator += (const quaternion& q)
{
	x += q.x;
	y += q.y;
	z += q.z;
	w += q.w;
	return *this;
}

quaternion& quaternion::operator -= (const quaternion& q)
{
	x -= q.x;
	y -= q.y;
	z -= q.z;
	w -= q.w;
	return *this;
}

quaternion quaternion::operator + (const quaternion &q) const
{
	quaternion r;
	r.x = q.x + x;
	r.y = q.y + y;
	r.z = q.z + z;
	r.w = q.w + w;
	return r;
}

quaternion quaternion::operator - (const quaternion &q) const
{
	quaternion r;
	r.x = q.x - x;
	r.y = q.y - y;
	r.z = q.z - z;
	r.w = q.w - w;
	return r;
}

quaternion quaternion::operator * (float f) const
{
	quaternion r;
	r.x = x * f;
	r.y = y * f;
	r.z = z * f;
	r.w = w * f;
	return r;
}

quaternion quaternion::operator * (const quaternion &q) const
{
	quaternion r;
	r.w = w*q.w - x*q.x - y*q.y - z*q.z;
	r.x = w*q.x + x*q.w + y*q.z - z*q.y;
	r.y = w*q.y + y*q.w + z*q.x - x*q.z;
	r.z = w*q.z + z*q.w + x*q.y - y*q.x;
	return r;
}

vector quaternion::operator * (const vector &v) const
{
	vector r = v * (w*w - x*x - y*y - z*z);
	vector *vv = (vector*)&x;
	r += 2 * w * (*vv) ^ v;
	r += 2 * ((*vv) * v) * (*vv);
	return r;
}

string quaternion::str() const
{
	return format("(%f, %f, %f, %f)", x, y, z, w);
}


// kaba
void quaternion::imul(const quaternion &q)
{	*this = (*this) * q;	}

quaternion quaternion::mul(const quaternion &q) const
{	return (*this) * q;	}

void QuaternionIdentity(quaternion &q)
{
	q.w=1;
	q.x=q.y=q.z=0;
}

// rotation with an <angle w> and an <axis axis>
void QuaternionRotationA(quaternion &q,const vector &axis,float w)
{
	float w_half=w*0.5f;
	float s=sinf(w_half);
	q.w=cosf(w_half);
	q.x=axis.x*s;
	q.y=axis.y*s;
	q.z=axis.z*s;
}

// ZXY -> everything IN the game (world transformation)
void QuaternionRotationV(quaternion &q,const vector &ang)
{
	float wx_2=ang.x*0.5f;
	float wy_2=ang.y*0.5f;
	float wz_2=ang.z*0.5f;
	float cx=cosf(wx_2);
	float cy=cosf(wy_2);
	float cz=cosf(wz_2);
	float sx=sinf(wx_2);
	float sy=sinf(wy_2);
	float sz=sinf(wz_2);
	q.w=(cy*cx*cz) + (sy*sx*sz);
	q.x=(cy*sx*cz) + (sy*cx*sz);
	q.y=(sy*cx*cz) - (cy*sx*sz);
	q.z=(cy*cx*sz) - (sy*sx*cz);

	/*quaternion x,y,z;
	QuaternionRotationA(x,vector(1,0,0),ang.x);
	QuaternionRotationA(y,vector(0,1,0),ang.y);
	QuaternionRotationA(z,vector(0,0,1),ang.z);
	// y*x*z
	QuaternionMultiply(q,x,z);
	QuaternionMultiply(q,y,q);*/
}

// create a quaternion from a (rotation-) matrix
void QuaternionRotationM(quaternion &q, const matrix &m)
{
	float tr = m._00 + m._11 + m._22;
	float w = acosf((tr - 1) / 2);
	if ((w < 0.00000001f) && (w > -0.00000001f))
		QuaternionIdentity(q);
	else{
		float s = 0.5f / sinf(w);
		vector n;
		n.x = (m._21 - m._12) * s;
		n.y = (m._02 - m._20) * s;
		n.z = (m._10 - m._01) * s;
		n.normalize();
		QuaternionRotationA(q, n, w);
	}
}

// invert a quaternion rotation
void quaternion::invert()
{
	x = -x;
	y = -y;
	z = -z;
}

quaternion quaternion::bar() const
{
	return quaternion(w, vector(-x, -y, -z));
}

// unite 2 rotations (first rotate by q1, then by q2: q = q2*q1)
void QuaternionMultiply(quaternion &q,const quaternion &q2,const quaternion &q1)
{
	quaternion _q;
	_q.w = q2.w*q1.w - q2.x*q1.x - q2.y*q1.y - q2.z*q1.z;
	_q.x = q2.w*q1.x + q2.x*q1.w + q2.y*q1.z - q2.z*q1.y;
	_q.y = q2.w*q1.y + q2.y*q1.w + q2.z*q1.x - q2.x*q1.z;
	_q.z = q2.w*q1.z + q2.z*q1.w + q2.x*q1.y - q2.y*q1.x;
	q=_q;
}

// q = q1 + t*( q2 - q1)
void QuaternionInterpolate(quaternion &q,const quaternion &q1,const quaternion &q2,float t)
{
	//msg_todo("TestMe: QuaternionInterpolate(2q) for OpenGL");
	q=q1;

	t=1-t; // ....?

	// dot product = cos angle(q1,q2)
	float c = q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
	float t2;
	bool flip=false;
	// flip, if q1 and q2 on opposite hemispheres
	if (c<0){
		c=-c;
		flip=true;
	}
	// q1 and q2 "too equal"?
	if (c>0.9999f)
		t2=1.0f-t;
	else{
		float theta=acosf(c);
		float phi=theta;//+spin*pi; // spin for additional circulations...
		float s=sinf(theta);
		t2=sinf(theta-t*phi)/s;
		t=sinf(t*phi)/s;
	}
	if (flip)
		t=-t;

	q.x = t*q1.x + t2*q2.x;
	q.y = t*q1.y + t2*q2.y;
	q.z = t*q1.z + t2*q2.z;
	q.w = t*q1.w + t2*q2.w;
}

void QuaternionInterpolate(quaternion &q,const quaternion &q1,const quaternion &q2,const quaternion &q3,const quaternion &q4,float t)
{
	/*#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQUATERNION A,B,C;
		D3DXQuaternionSquadSetup(&A,&B,&C,(D3DXQUATERNION*)&q1,(D3DXQUATERNION*)&q2,(D3DXQUATERNION*)&q3,(D3DXQUATERNION*)&q4);
		D3DXQuaternionSquad((D3DXQUATERNION*)&q,(D3DXQUATERNION*)&q2,&A,&B,&C,t);
	#else*/
	q=q2;
	msg_todo("QuaternionInterpolate(4q)");
}

// convert a quaternion into 3 angles (ZXY)
vector quaternion::get_angles() const
{
	vector ang;
	ang.x = asin(2*(w*x - z*y));
	ang.y = atan2(2*(w*y+x*z), 1-2*(y*y+x*x));
	ang.z = atan2(2*(w*z+x*y), 1-2*(z*z+x*x));
/*	// really bad!
	vector ang,v;
	matrix m,x,y;
	MatrixRotationQ(m,*this);
	v=m*vector(0,0,1000.0f);
	ang.y= atan2f(v.x,v.z);
	ang.x=-atan2f(v.y,sqrt(v.x*v.x+v.z*v.z));
	MatrixRotationX(x,-ang.x);
	MatrixRotationY(y,-ang.y);
	MatrixMultiply(m,y,m);
	MatrixMultiply(m,x,m);
	v=m*vector(1000.0f,0,0);
	ang.z=atan2f(v.y,v.x);*/
	return ang;
}

// scale the angle of the rotation
void QuaternionScale(quaternion &q,float f)
{
	float w=q.get_angle();
	if (w==0)	return;

	q.w=cosf(w*f/2);
	float factor=sinf(w*f/2)/sinf(w/2);
	q.x *= factor;
	q.y *= factor;
	q.z *= factor;
}

// quaternion correction
void quaternion::normalize()
{
	float l = sqrtf(x*x + y*y + z*z + w*w);
	l = 1.0f / l;
	w *= l;
	x *= l;
	y *= l;
	z *= l;
}

// the axis of our quaternion rotation
vector quaternion::get_axis() const
{
	vector ax = vector(x, y, z);
	ax.normalize();
	return ax;
}

// angle value of the quaternion
float quaternion::get_angle() const
{
	return acosf(w)*2;
}

void QuaternionDrag(quaternion &q, const vector &up, const vector &dang, bool reset_z)
{
	quaternion T, TT;
	bool is_not_z = (up.x != 0) || (up.y != 0) || (up.z < 0);
	if (is_not_z){
		vector ax = vector::EZ ^ up;
		ax.normalize();
		vector up2 = up;
		up2.normalize();
		QuaternionRotationA(T, ax, acos(up2.z));
		TT = T.bar();
		q = T * q * TT;
	}

	vector ang = q.get_angles();
	ang.x = clampf(ang.x + dang.x, -pi/2+0.01f, pi/2-0.01f);
	ang.y += dang.y;
	ang.z += dang.z;
	if (reset_z)
		ang.z = 0;
	QuaternionRotationV(q, ang);

	if (is_not_z)
		q = TT * q * T;
}
