#pragma once
#ifndef ITKHELPER_H
#define ITKHELPER_H

#include <QMap>
#include <QString>
#include <string.h>
QMap<QString, QString> getMetaInfoFromCTFile(const char* path);

#endif