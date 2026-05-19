#pragma once

namespace qtlogin::sdologin {

inline const char* qtLoginHighDpiScalingEnvValue()
{
    return "0";
}

inline const char* qtLoginAutoScreenScaleFactorEnvValue()
{
    return "0";
}

inline bool qtLoginShouldForce96Dpi()
{
    return true;
}

void configureQtLoginDpi();

}
