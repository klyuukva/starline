#include <QCoreApplication>
#include <QDebug>
#include <QDateTime>
#include <QFile>

const int TWO_MINUTES = 120;
const QTime START_TIME(0,0,0);
const QTime END_TIME(23,59,59);


struct TransportInfo{
    QDateTime  date_time;
    QString id;
    int speed{};
};

struct DriveInfo{

    QString id;
    QTime travel_time;
    QTime parking_time;

    QDateTime prev_datetime;
    int prev_speed;

    QDateTime zero_speed_time;

    uint qHash(const DriveInfo &key);

    bool operator==(const DriveInfo &rhs) const {
        return id == rhs.id &&
               travel_time == rhs.travel_time &&
               parking_time == rhs.parking_time &&
               prev_datetime == rhs.prev_datetime &&
               prev_speed == rhs.prev_speed;
    }

    bool operator!=(const DriveInfo &rhs) const {
        return !(rhs == *this);
    }


    QString toString() const {

            return QString("id: %1\nвремя пути: %2\nвремя стоянки: %3\n----\n")
                    .arg(id)
                    .arg(travel_time.toString())
                    .arg(parking_time.toString());
        }

};

uint qHash(const DriveInfo &key)
{
    return qHash(key.id);
}

//Функция, которая считывает .csv файл
QVector<TransportInfo> parseFile(const QString &filePath){

    QVector<TransportInfo> transport_vector;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file.";
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();

        QStringList fields = line.split(",");
        if (fields.size() < 3) {
            qWarning() << "Invalid string format";
            continue;
        }

        QString format = "yyyy-MM-dd hh:mm:ss";

        TransportInfo transport;
        transport.date_time = QDateTime::fromString(fields[0].mid(1, fields[0].length() - 2), "yyyy-MM-dd hh:mm:ss");
        transport.id = fields[1];
        transport.speed = fields[2].toInt();

        transport_vector.append(transport);
    }

    file.close();

    return transport_vector;
};

//Функция, которая считает время отсановки и время передвижения транспорта
QHash<QString,DriveInfo> calcDrivesStat(const QVector<TransportInfo>& transport_vector){
    QHash<QString,DriveInfo> drive_hash;

    for (const auto & i : transport_vector) {
        DriveInfo drive;

        if(!drive_hash.contains(i.id)) {
            drive.id = i.id;
            drive.parking_time = QTime(0,0,0);
            drive.travel_time = QTime(0,0,0);

            drive.prev_datetime = i.date_time;
            drive.prev_speed = i.speed;
            drive.zero_speed_time = i.date_time;
            drive_hash.insert(i.id, drive);
        } else {

            drive = drive_hash[i.id];

            //двигвлся - стоит
            if (i.speed == 0 && drive.prev_speed != 0) {
                auto time_difference = drive.prev_datetime.time().secsTo(i.date_time.time());
                drive.travel_time = drive.travel_time.addSecs(time_difference);

                drive.zero_speed_time = i.date_time;
                drive.prev_speed = i.speed;
                drive.prev_datetime = i.date_time;
            }

            //стоял - стоит
            if (i.speed == 0 && drive.prev_speed == 0) {
                drive.prev_speed = i.speed;
                drive.prev_datetime = i.date_time;
            }

            //стоял - двигается
            if (i.speed != 0 && drive.prev_speed == 0) {
                auto zero_time_difference = drive.zero_speed_time.time().secsTo(i.date_time.time());

                if(zero_time_difference > TWO_MINUTES){
                    drive.parking_time = drive.parking_time.addSecs(zero_time_difference);
                } else {
                    drive.travel_time = drive.travel_time.addSecs(zero_time_difference);
                }

                drive.prev_speed = i.speed;
                drive.prev_datetime = i.date_time;
            }

            //двигался - двигается
            if (i.speed != 0 && drive.prev_speed != 0) {
                drive.travel_time = drive.travel_time.addSecs(drive.prev_datetime.time().secsTo(i.date_time.time()));
                drive.prev_speed = i.speed;
                drive.prev_datetime = i.date_time;
            }

            drive_hash.insert(i.id,drive);
        }


    }

    for(auto & di : drive_hash) {
        auto sumTime = di.parking_time.addSecs(START_TIME.secsTo(di.travel_time));
        auto time_difference = sumTime.secsTo(END_TIME);
        if(time_difference > TWO_MINUTES){
            di.parking_time = di.parking_time.addSecs(time_difference);
        } else {
            di.travel_time = di.travel_time.addSecs(time_difference);
        }

    }

    return drive_hash;
};

//Функция, которая записывает статистику в .txt файл
void writeQHashToFile(const QHash<QString, DriveInfo> &hash, const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Не удалось открыть файл для записи:" << fileName;
        return;
    }

    QTextStream out(&file);
    QHash<QString, DriveInfo>::const_iterator it;
    for (it = hash.constBegin(); it != hash.constEnd(); ++it) {
        out << it.value().toString();
    }

    file.close();
}


int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QStringList arguments = QCoreApplication::arguments();

    QString pathIn = arguments.at(1);

    QString pathOut = arguments.at(2);

    QVector<TransportInfo> test_transport = parseFile(pathIn);

    QHash<QString,DriveInfo> test_hash = calcDrivesStat(test_transport);

    writeQHashToFile(test_hash,pathOut);

    return 0;
}
