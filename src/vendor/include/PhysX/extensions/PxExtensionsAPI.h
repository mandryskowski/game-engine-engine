//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef PX_EXTENSIONS_API_H
#define PX_EXTENSIONS_API_H
/** \addtogroup extensions
  @{
*/

#include <PhysX/foundation/PxErrorCallback.h>
#include <PhysX/extensions/PxDefaultAllocator.h>
#include <PhysX/extensions/PxConstraintExt.h>
#include <PhysX/extensions/PxDistanceJoint.h>
#include <PhysX/extensions/PxContactJoint.h>
#include <PhysX/extensions/PxFixedJoint.h>
#include <PhysX/extensions/PxPrismaticJoint.h>
#include <PhysX/extensions/PxRevoluteJoint.h>
#include <PhysX/extensions/PxSphericalJoint.h>
#include <PhysX/extensions/PxD6Joint.h>
#include <PhysX/extensions/PxDefaultSimulationFilterShader.h>
#include <PhysX/extensions/PxDefaultErrorCallback.h>
#include <PhysX/extensions/PxDefaultStreams.h>
#include <PhysX/extensions/PxRigidActorExt.h>
#include <PhysX/extensions/PxRigidBodyExt.h>
#include <PhysX/extensions/PxShapeExt.h>
#include <PhysX/extensions/PxTriangleMeshExt.h>
#include <PhysX/extensions/PxSerialization.h>
#include <PhysX/extensions/PxDefaultCpuDispatcher.h>
#include <PhysX/extensions/PxSmoothNormals.h>
#include <PhysX/extensions/PxSimpleFactory.h>
#include <PhysX/extensions/PxStringTableExt.h>
#include <PhysX/extensions/PxBroadPhaseExt.h>
#include <PhysX/extensions/PxMassProperties.h>
#include <PhysX/extensions/PxSceneQueryExt.h>

/** \brief Initialize the PhysXExtensions library. 

This should be called before calling any functions or methods in extensions which may require allocation. 
\note This function does not need to be called before creating a PxDefaultAllocator object.

\param physics a PxPhysics object
\param pvd an PxPvd (PhysX Visual Debugger) object

@see PxCloseExtensions PxFoundation PxPhysics
*/

PX_C_EXPORT bool PX_CALL_CONV PxInitExtensions(physx::PxPhysics& physics, physx::PxPvd* pvd);

/** \brief Shut down the PhysXExtensions library. 

This function should be called to cleanly shut down the PhysXExtensions library before application exit. 

\note This function is required to be called to release foundation usage.

@see PxInitExtensions
*/

PX_C_EXPORT void PX_CALL_CONV PxCloseExtensions();

/** @} */
#endif // PX_EXTENSIONS_API_H
