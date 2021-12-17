#ifndef TVMGEN_DEFAULT_H_
#define TVMGEN_DEFAULT_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Workspace size for TVM module "default"
 */
#define TVMGEN_DEFAULT_WORKSPACE_SIZE 20*1024

/*!
 * \brief entrypoint function for TVM module "default"
 * \param inputs Input tensors for the module 
 * \param outputs Output tensors for the module 
 */
int32_t tvmgen_default_run_model(float* inputs, float* outputs);

#ifdef __cplusplus
}
#endif

#endif // TVMGEN_DEFAULT_H_
