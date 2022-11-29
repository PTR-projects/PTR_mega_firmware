/*
 * quaternion.c
 *
 *  Created on: 24 lis 2022
 *      Author: bartek
 */
#include <stdio.h>
#include <math.h>
#include "common.h"
#include "quaternion.h"

void quaternionInitUnit(quaternions_t * result){
    result->q0 = 1.0f;
    result->q1 = 0.0f;
    result->q2 = 0.0f;
    result->q3 = 0.0f;
}

void quaternionInitFromVector(quaternions_t * result, quaternions_t * v){
    result->q0 = 0.0f;
    result->q1 = v->x;
    result->q2 = v->y;
    result->q3 = v->z;
}

float quaternionNormSqared(quaternions_t * q){
    return POW2(q->q0) + POW2(q->q1) + POW2(q->q2) + POW2(q->q3);
}

void quaternionMultiply(quaternions_t * result, quaternions_t * a, quaternions_t * b){
	quaternions_t p;

	p.q0 = a->q0 * b->q0 - a->q1 * b->q1 - a->q2 * b->q2 - a->q3 * b->q3;
	p.q1 = a->q0 * b->q1 + a->q1 * b->q0 + a->q2 * b->q3 - a->q3 * b->q2;
	p.q2 = a->q0 * b->q2 - a->q1 * b->q3 + a->q2 * b->q0 + a->q3 * b->q1;
	p.q3 = a->q0 * b->q3 + a->q1 * b->q2 - a->q2 * b->q1 + a->q3 * b->q0;

	*result = p;
}

void quaternionScale(quaternions_t * result, quaternions_t * a, float b){
	quaternions_t p;

    p.q0 = a->q0 * b;
    p.q1 = a->q1 * b;
    p.q2 = a->q2 * b;
    p.q3 = a->q3 * b;

    *result = p;
}

void quaternionAdd(quaternions_t * result, quaternions_t * a, quaternions_t * b){
	quaternions_t p;

    p.q0 = a->q0 + b->q0;
    p.q1 = a->q1 + b->q1;
    p.q2 = a->q2 + b->q2;
    p.q3 = a->q3 + b->q3;

    *result = p;
}

void quaternionConjugate(quaternions_t * result, quaternions_t * q){
    result->q0 =  q->q0;
    result->q1 = -q->q1;
    result->q2 = -q->q2;
    result->q3 = -q->q3;
}

void quaternionNormalize(quaternions_t * result, quaternions_t * q){
    float mod = sqrtf(quaternionNormSqared(q));
    if (mod < 1e-6f) {
        // Length is too small - re-initialize to zero rotation
        result->q0 = 1;
        result->q1 = 0;
        result->q2 = 0;
        result->q3 = 0;
    }
    else {
        result->q0 = q->q0 / mod;
        result->q1 = q->q1 / mod;
        result->q2 = q->q2 / mod;
        result->q3 = q->q3 / mod;
    }
}

void quaternionRotateVector(vectorf_t * result, quaternions_t * vect, quaternions_t * ref){
	quaternions_t vectQuat, refConj;

    vectQuat.q0 = 0;
    vectQuat.q1 = vect->x;
    vectQuat.q2 = vect->y;
    vectQuat.q3 = vect->z;

    quaternionConjugate(&refConj, ref);
    quaternionMultiply(&vectQuat, &refConj, &vectQuat);
    quaternionMultiply(&vectQuat, &vectQuat, ref);

    result->x = vectQuat.q1;
    result->y = vectQuat.q2;
    result->z = vectQuat.q3;
}

void quaternionRotateVectorInv(vectorf_t * result, vectorf_t * vect, quaternions_t * ref){
	quaternions_t vectQuat, refConj;

    vectQuat.q0 = 0;
    vectQuat.q1 = vect->x;
    vectQuat.q2 = vect->y;
    vectQuat.q3 = vect->z;

    quaternionConjugate(&refConj, ref);
    quaternionMultiply(&vectQuat, ref, &vectQuat);
    quaternionMultiply(&vectQuat, &vectQuat, &refConj);

    result->x = vectQuat.q1;
    result->y = vectQuat.q2;
    result->z = vectQuat.q3;
}

void quaternionComputeProducts(quaternions_t *quat, quaternionsProd_t *quatProd){
    quatProd->ww = quat->w * quat->w;
    quatProd->wx = quat->w * quat->x;
    quatProd->wy = quat->w * quat->y;
    quatProd->wz = quat->w * quat->z;
    quatProd->xx = quat->x * quat->x;
    quatProd->xy = quat->x * quat->y;
    quatProd->xz = quat->x * quat->z;
    quatProd->yy = quat->y * quat->y;
    quatProd->yz = quat->y * quat->z;
    quatProd->zz = quat->z * quat->z;
}

