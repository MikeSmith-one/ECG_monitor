#pragma once

#include <QMetaType>

//波形图所需数据包
struct ChartsData {
	qint16 ECG_Data;		//心电波形数值
	quint8 SpO2_Data;		//血氧波形数值
	quint8 RESP_Data;		//呼吸波形数值
	quint8 Status_Flag;	    //状态标志位
};

Q_DECLARE_METATYPE(ChartsData)

//参数表区域所需数据包
struct TablesData {
	quint8 HR_Value;		//心率数值，范围 0~250 bpm	
	quint8 SpO2_Value;		//血氧饱和度，范围 0~100 %
	quint8 RESP_Value;		//呼吸率，范围 0~100 rpm
	quint16 TEMP_value;		//体温
	quint8 NIBP_Sys;		//血压收缩压（高压），范围 0~250 mmHg
	quint8 NIBP_Dia;		//血压舒张压（低压），范围 0~250 mmHg
};

Q_DECLARE_METATYPE(TablesData)