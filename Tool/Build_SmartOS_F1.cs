using System;
using System.Collections;
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Linq;
using System.IO;
using System.Collections.Generic;
using Microsoft.Win32;
using NewLife.Log;

namespace NewLife.Reflection
{
    public class ScriptEngine
    {
        static void Main()
        {
            XTrace.UseConsole();

            var build = new Builder();
            build.Init();
			build.Cortex = 3;
			build.AddIncludes("..\\..\\Lib");
            build.AddFiles("..\\", "*.c;*.cpp", false, "CAN;DMA;Memory;String");
            build.AddFiles("..\\Platform", "Boot_F1.cpp");
            build.AddFiles("..\\Platform", "startup_stm32f10x.s");
            build.AddFiles("..\\Security");
            build.AddFiles("..\\App");
            build.AddFiles("..\\Drivers");
            build.AddFiles("..\\Net");
            build.AddFiles("..\\TinyIP", "*.c;*.cpp", false, "HttpClient");
            build.AddFiles("..\\Message");
            build.AddFiles("..\\TinyNet");
            build.AddFiles("..\\TokenNet");
            build.CompileAll();
            build.BuildLib("..\\SmartOS_F1");

			build.Debug = false;
            build.CompileAll();
            build.BuildLib("..\\SmartOS_F1");
        }
    }

}
	//include=MDK.cs
