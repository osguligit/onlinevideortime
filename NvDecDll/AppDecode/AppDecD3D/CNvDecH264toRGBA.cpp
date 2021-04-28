#include "CNvDecH264toRGBA.h"

CNvDecH264RGBA::CNvDecH264RGBA(int _iGpu)
{
	m_iGpu = _iGpu;
	cuDevice = 0;
	ck(cuDeviceGet(&cuDevice, m_iGpu));
	ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
	std::cout << "GPU in use: " << szDeviceName << std::endl;

	cuContext = NULL;
	ck(cuCtxCreate(&cuContext, CU_CTX_SCHED_BLOCKING_SYNC, cuDevice));
}

CNvDecH264RGBA::~CNvDecH264RGBA()
{

}

void CNvDecH264RGBA::CNvDecH264RGBA_Decode()
{
}

void CNvDecH264RGBA::CNvDecH264RGBA_NV12toRGBA()
{
}
