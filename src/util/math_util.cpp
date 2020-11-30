#include "math_util.h"

glm::quat math_util::safeQuatLookAt(glm::vec3 const & lookFrom,
                         glm::vec3 const & lookTo,
                         glm::vec3 const & up,
                         glm::vec3 const & alternativeUp) {
    glm::vec3  direction       = lookTo - lookFrom;
    float      directionLength = glm::length(direction);

    // Check if the direction is valid; Also deals with NaN
    if(!(directionLength > 0.0001))
        return glm::quat(1, 0, 0, 0); // Just return identity

    // Normalize direction
    direction /= directionLength;

    // Is the normal up (nearly) parallel to direction?
    if(glm::abs(glm::dot(direction, up)) > .9999f) {
        // Use alternative up
        return glm::quatLookAt(direction, alternativeUp);
    } else {
        return glm::quatLookAt(direction, up);
    }
}

glm::mat4 math_util::safeLookAt(glm::vec3 const & lookFrom,
                         		glm::vec3 const & lookTo,
                         		glm::vec3 const & up,
                         		glm::vec3 const & alternativeUp) {
    return glm::toMat4(safeQuatLookAt(lookFrom, lookTo, up, alternativeUp));
}

// thank you Joachim Kopp: http://www.mpi-hd.mpg.de/personalhomes/globes/3x3/
glm::mat3 math_util::diagonalizer(glm::mat3 const & A) {
	// A must be a symmetric matrix.
	// returns matrix Q 
	// can be used to Diagonalize A
	// Diagonal matrix D = Q * A * Transpose(Q);  and  A = QT*D*Q
	// The rows of q are the eigenvectors D's diagonal is the eigenvalues
	// As per 'row' convention if float3x3 Q = q.getmatrix(); then v*Q = q*v*conj(q)
	int maxsteps=24;  // certainly wont need that many.
	int i;
	glm::quat q(0.0f,0.0f,0.0f,1.0f);
	for(i = 0;i < maxsteps; ++i) {
		glm::mat3 Q  = glm::toMat3(q); // v*Q == q*v*conj(q)
		glm::mat3 D  = A * glm::transpose(Q) * Q;  // A = Q^T*D*Q
		glm::vec3 offdiag(D[1][2],D[0][2],D[0][1]); // elements not on the diagonal
		glm::vec3 om(fabsf(offdiag.x),fabsf(offdiag.y),fabsf(offdiag.z)); // mag of each offdiag elem
		int k = (om.x > om.y && om.x > om.z) ? 0 : (om.y > om.z)? 1 : 2; // index of largest element of offdiag
		int k1 = (k + 1) % 3;
		int k2 = (k + 2) % 3;
		if(offdiag[k]==0.0f) break;  // diagonal already
		float thet = (D[k2][k2]-D[k1][k1])/(2.0f*offdiag[k]);
		float sgn = (thet > 0.0f)?1.0f:-1.0f;
		thet    *= sgn; // make it positive
		float t = sgn /(thet +((thet < 1.E6f)?sqrtf((thet*thet)+1.0f):thet)) ; // sign(T)/(|T|+sqrt(T^2+1))
		float c = 1.0f/sqrtf((t*t)+1.0f); //  c= 1/(t^2+1) , t=s/c 
		if(c==1.0f) break;  // no room for improvement - reached machine precision.
		glm::quat jr(0,0,0,0); // jacobi rotation for this iteration.
		jr[k] = sgn*sqrtf((1.0f-c)/2.0f);  // using 1/2 angle identity sin(a/2) = sqrt((1-cos(a))/2)  
		jr[k] *= -1.0f; // since our quat-to-matrix convention was for v*M instead of M*v
		jr.w  = sqrtf(1.0f - (jr[k]*jr[k]));
		if(jr.w==1.0f) break; // reached limits of floating point precision
		q =  q*jr;  
		q = glm::normalize(q);
	} 
	return glm::toMat3(q);
  }
