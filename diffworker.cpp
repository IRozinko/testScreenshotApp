#include "diffworker.h"

DiffWorker::DiffWorker(QObject *parent) : QObject(parent)
{
}

QByteArray calculateHash(const QPixmap &pixmap)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    QByteArray hash = QCryptographicHash::hash(byteArray, QCryptographicHash::Sha256);
    return hash;
}

double calculateDifferencePercentage(const QByteArray &hash1, const QByteArray &hash2)
{
    int differenceBits = 0;
    int totalBits = hash1.length() * 8;

    for (int i = 0; i < hash1.length(); ++i)
    {
        uchar xorResult = hash1[i] ^ hash2[i];
        differenceBits += std::bitset<8>(xorResult).count();
    }

    double differencePercentage = (static_cast<double>(differenceBits) / totalBits) * 100;
    return differencePercentage;
}

void DiffWorker::process(const QPixmap &screenshot1, const QPixmap &screenshot2)
{
    QByteArray hash1 = calculateHash(screenshot1);
    QByteArray hash2 = calculateHash(screenshot2);
    double differencePercentage = calculateDifferencePercentage(hash1, hash2);

    emit hashCalculated(hash1);
    emit finished(differencePercentage);
}
