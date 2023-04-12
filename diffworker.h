#ifndef DIFFWORKER_H
#define DIFFWORKER_H

#include <QObject>
#include <QPixmap>
#include <QCryptographicHash>
#include <QBuffer>
#include <bitset>

class DiffWorker : public QObject
{
    Q_OBJECT
public:
    explicit DiffWorker(QObject *parent = nullptr);

public slots:
    void process(const QPixmap &screenshot1, const QPixmap &screenshot2);

signals:
    void finished(double differencePercentage);
    void hashCalculated(const QByteArray &hash);
};

#endif // DIFFWORKER_H
