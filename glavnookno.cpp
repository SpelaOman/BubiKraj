#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QtSql>
#include <QMessageBox>

#include "glavnookno.h"
#include "ui_glavnookno.h"

GlavnoOkno::GlavnoOkno(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GlavnoOkno) {
    ui->setupUi(this);

    ui->txt_pot_izvorne->setVisible(false);
    ui->txt_pot_izhodna->setVisible(false);

    preveri_izvorno();
    preveri_izhodno();

}

GlavnoOkno::~GlavnoOkno() {
    delete ui;
}

void GlavnoOkno::on_btn_izhod_clicked() {

   close();

}

void GlavnoOkno::on_btn_izberi_izvorno_datoteko_clicked() {

    QString pot_do_datoteke = "";
    pot_do_datoteke = QFileDialog::getOpenFileName(this, "Izberite izvorno datoteko", QDir::homePath(), "Tekstovna datoteka (*.csv *.txt);;Vse datoteke (*.*)");
    ui->txt_pot_izvorne->setText(pot_do_datoteke);

    preveri_izvorno();

}

void GlavnoOkno::on_btn_izberi_izhodno_datoteko_clicked() {

    QString pot_do_datoteke = "";
    pot_do_datoteke = QFileDialog::getSaveFileName(this, "Izberite izhodno datoteko", QDir::homePath() + "/izhodna.csv", "Tekstovna datoteka (*.csv *.txt);;Vse datoteke (*.*)");
    ui->txt_pot_izhodna->setText(pot_do_datoteke);

    preveri_izhodno();

}

void GlavnoOkno::on_btn_posodobi_regije_obcine_clicked() {

    // nastavi URL spletne strani
    QString pot = "http://sl.wikipedia.org/wiki/Statisti%C4%8Dne_regije_Slovenije";

    // povezi s spletnim naslovom in pocakaj na odziv
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(konec_odziva_regije(QNetworkReply*)));

    QUrl url(pot);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent" , "Mozilla Firefox" );
    request.setRawHeader("Accept-Charset", "windows-1250,ISO-8859-1,utf-8;q=0.7,*;q=0.7");
    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    request.setRawHeader("Accept-Language", "en-us,en;q=0.5");
    request.setRawHeader("Connection", "Keep-Alive");
    manager->get(request);

}

void GlavnoOkno::on_btn_zazeni_clicked() {

        /**
          * Odpremo seznam tipa CSV, ki ga tvori Google Analytics
          **/

        QFile izvorna_datoteka(ui->txt_pot_izvorne->text());
        if (!izvorna_datoteka.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }
        QTextStream izvorni_seznam(&izvorna_datoteka);

        /**
         * Odpremo novo datoteko tipa CSV, ki jo tvori program in ustreza strukturi Google Analytics-a za obdelavo
         **/

        QFile izhodna_datoteka(ui->txt_pot_izhodna->text());
        if (!izhodna_datoteka.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return;
        }
        QTextStream izhodni_seznam(&izhodna_datoteka);

        QString star_seznam = "";
        QString nov_seznam = "";

        /**
          * Odpremo bazo podatkov
          **/

        QString app_path = QApplication::applicationDirPath();
        QString dbase_path = app_path + "/base.bz";

        {
            QSqlDatabase base = QSqlDatabase::addDatabase("QSQLITE");
            base.setDatabaseName(dbase_path);
            base.database();
            base.open();
            if ( base.isOpen() != true ) {
                QMessageBox msgbox;
                msgbox.setText("There was a problem with the database");
                msgbox.setInformativeText("Due to unknown reason there was a problem with opening the database.\nThe problem accured during initial opening of the database.");
                msgbox.exec();
            }
            else {
                // the database is open

                do {

                    nov_seznam = "";
                    star_seznam = "";

                    // preberemo vrstico iz seznama
                    star_seznam = izvorni_seznam.readLine();

                    // pregledamo, ce je vrstica slucajno prazna ali komentar in jo prepisemo v novo datoteko
                    if ( star_seznam.startsWith(" ") || star_seznam.startsWith("#") || star_seznam.isEmpty() ) {
                        nov_seznam = star_seznam;
                    }
                    // pregledamo, ce vrstica slucajno nosi imena polj in dodamo novi polji
                    else if ( star_seznam.startsWith("Kraj") ) {
                        nov_seznam = "Regija,Obcina," + star_seznam;
                    }
                    // ce ni, iz nje izluscimo kraj in mu dodamo regijo in obcino
                    else {
                        QString kraj_ga = star_seznam.left(star_seznam.indexOf(","));

                        QSqlQuery sql_kraj;
                        sql_kraj.prepare("SELECT * FROM kraji WHERE kraj_ga LIKE '" + kraj_ga + "'");
                        sql_kraj.exec();
                        if ( sql_kraj.next() ) {
                            QString regija = sql_kraj.value(sql_kraj.record().indexOf("regija")).toString();
                            QString obcina = sql_kraj.value(sql_kraj.record().indexOf("obcina")).toString();
                            QString kraj = sql_kraj.value(sql_kraj.record().indexOf("kraj")).toString();

                            nov_seznam = regija + "," + obcina + "," + kraj + "," + star_seznam.right(star_seznam.length() - star_seznam.indexOf(",") - 1);
                        }
                        else {
                            nov_seznam = "Neznano,Neznano," + star_seznam;
                            ui->txt_output->setPlainText("Neznan kraj: " + kraj_ga + "\n" + ui->txt_output->toPlainText());
                        }
                    }

                    // v novo datoteko vnesemo nove podatke
                    izhodni_seznam << nov_seznam + "\n";

                } while (!star_seznam.isNull());

            }
            base.close();
        }

        izvorna_datoteka.close();
        izhodna_datoteka.close();

}

void GlavnoOkno::konec_odziva_regije(QNetworkReply *odgovor) {

    QString nov_odgovor = odgovor->readAll();

    int st_regij = 0;

    QString regije = "";
    QString obcine = "";
    QString kraji = "";

    // skrajsamo pridobljeno besedilo na zacetek seznama regij
    nov_odgovor = nov_odgovor.right(nov_odgovor.length() - nov_odgovor.indexOf("Statistične regije</div>"));
    nov_odgovor = nov_odgovor.right(nov_odgovor.length() - nov_odgovor.indexOf("<ol>") - 4);

    // pridobimo surovo besedilo, ki vsebuje seznam regij
    regije = nov_odgovor.left(nov_odgovor.indexOf("</ol>"));

    // skrajsamo besedilo za seznam regij
    nov_odgovor = nov_odgovor.right(nov_odgovor.length() - regije.length());
    nov_odgovor = nov_odgovor.left(nov_odgovor.indexOf("Glej tudi"));

    st_regij = regije.count("<li>");

    QStringList seznam_regij;

    // olepsamo surovo besedilo, ki vsebuje seznam regij
    regije = regije.remove("<li><a href");
    regije = regije.replace("</a></li>\n", ",");

    int mesto_regije = 0;

    // izluscimo posamezne regije in jih napolnimo v seznam
    for ( int i = 0; i < st_regij; i++ ) {
        QString regija = "";
        regija = regije.right(regije.length() - regije.indexOf(">", mesto_regije) - 1 );
        regija = regija.left(regija.indexOf(","));
        seznam_regij.append(regija);
        mesto_regije = regije.indexOf(regija) + regija.length() + 2;
    }

    QStringList seznam_obcin;

    for ( int i = 0; i < seznam_regij.count(); i++ ) { // v vsaki regiji doloci obcine
        // iz vseh seznamov izluscimo seznam obcin za dano regijo
        QString obcine_v_regiji = nov_odgovor.right(nov_odgovor.length() - nov_odgovor.indexOf("<table>"));
        obcine_v_regiji = obcine_v_regiji.left(obcine_v_regiji.indexOf("</table>"));

        nov_odgovor = nov_odgovor.right(nov_odgovor.length() - 8 - nov_odgovor.indexOf("</table>"));

        // prestejemo stevilo obcin
        int st_obcin = obcine_v_regiji.count("<li>");
        int mesto_obcine = 0;

        for ( int j = 0; j < st_obcin; j++ ) {
            QString url = obcine_v_regiji.right(obcine_v_regiji.length() - obcine_v_regiji.indexOf("a href=", mesto_obcine) - 8);
            url = url.left(url.indexOf("\""));
            mesto_obcine = obcine_v_regiji.indexOf(url);

            QString obcina = obcine_v_regiji.right(obcine_v_regiji.length() - obcine_v_regiji.indexOf(">", mesto_obcine) - 1);
            obcina = obcina.left(obcina.indexOf("</a></li>"));
            mesto_obcine = obcine_v_regiji.indexOf(obcina);

            seznam_obcin.append(seznam_regij.at(i) + ";" + obcina + ";http://sl.wikipedia.org" + url);

        }
    }

    QString app_path = QApplication::applicationDirPath();
    QString dbase_path = app_path + "/base.bz";

    {
        QSqlDatabase base = QSqlDatabase::addDatabase("QSQLITE");
        base.setDatabaseName(dbase_path);
        base.database();
        base.open();
        if ( base.isOpen() != true ) {
            QMessageBox msgbox;
            msgbox.setText("There was a problem with the database");
            msgbox.setInformativeText("Due to unknown reason there was a problem with opening the database.\nThe problem accured during initial opening of the database.");
            msgbox.exec();
        }
        else {
            // the database is open

            QSqlQuery create_table;
            create_table.prepare("CREATE TABLE IF NOT EXISTS regije ("
                                 "id INTEGER PRIMARY KEY, "
                                 "regija TEXT)");
            create_table.exec();

            for ( int i = 0; i < seznam_regij.count(); i++ ) {
                QSqlQuery check;
                check.prepare("SELECT * FROM regije WHERE regija LIKE '" + seznam_regij.at(i) + "'");
                check.exec();
                if ( !check.next() ) {
                    QSqlQuery enter;
                    enter.prepare("INSERT INTO regije (regija) VALUES (?)");
                    enter.bindValue(0, seznam_regij.at(i));
                    enter.exec();
                    ui->txt_output->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
                    ui->txt_output->insertPlainText("Vnos regije: " + seznam_regij.at(i) + "\n");
                }
                qApp->processEvents();
            }

            create_table.clear();
            create_table.prepare("CREATE TABLE IF NOT EXISTS obcine ("
                                 "id INTEGER PRIMARY KEY, "
                                 "regija TEXT, "
                                 "obcina TEXT, "
                                 "url TEXT)");
            create_table.exec();

            // prikaz obcin na ekranu
            for ( int i = 0; i < seznam_obcin.count(); i++ ) {
                QString regija = "";
                QString obcina = "";
                QString url = "";

                regija = seznam_obcin.at(i).left(seznam_obcin.at(i).indexOf(";"));
                obcina = seznam_obcin.at(i).right(seznam_obcin.at(i).length() - regija.length() - 1);
                obcina = obcina.left(obcina.indexOf(";"));
                url = seznam_obcin.at(i).right(seznam_obcin.at(i).length() - regija.length() - obcina.length() - 2);

                QSqlQuery check;
                check.prepare("SELECT * FROM obcine WHERE obcina LIKE '" + obcina + "'");
                check.exec();
                if ( !check.next() ) {
                    QSqlQuery enter;
                    enter.prepare("INSERT INTO obcine (regija, obcina, url) VALUES (?, ?, ?)");
                    enter.bindValue(0, regija);
                    enter.bindValue(1, obcina);
                    enter.bindValue(2, url);
                    enter.exec();
                    enter.clear();

                    ui->txt_output->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
                    ui->txt_output->insertPlainText("Vnos obcine: " + obcina + "\n");

                    qApp->processEvents();
                }
            }

            for ( int i = 0; i < seznam_obcin.count(); i++ ) {
                QString regija = "";
                QString obcina = "";
                QString pot = "";

                regija = seznam_obcin.at(i).left(seznam_obcin.at(i).indexOf(";"));
                obcina = seznam_obcin.at(i).right(seznam_obcin.at(i).length() - regija.length() - 1);
                obcina = obcina.left(obcina.indexOf(";"));
                pot = seznam_obcin.at(i).right(seznam_obcin.at(i).length() - regija.length() - obcina.length() - 2);


                // povezi s spletnim naslovom in pocakaj na odziv
                QNetworkAccessManager *manager = new QNetworkAccessManager(this);
                connect(manager, SIGNAL(finished(QNetworkReply*)),
                        this, SLOT(konec_odziva_kraji(QNetworkReply*)));

                QUrl url(pot);
                QNetworkRequest request(url);
                request.setRawHeader("User-Agent" , "Mozilla Firefox" );
                request.setRawHeader("Accept-Charset", "windows-1250,ISO-8859-1,utf-8;q=0.7,*;q=0.7");
                request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
                request.setRawHeader("Accept-Language", "en-us,en;q=0.5");
                request.setRawHeader("Connection", "Keep-Alive");
                manager->get(request);

            }

        }
        base.close();

   }

}

void GlavnoOkno::konec_odziva_kraji(QNetworkReply *odgovor) {

    QString nov_odgovor = odgovor->readAll();

    nov_odgovor = nov_odgovor.right(nov_odgovor.length() - nov_odgovor.indexOf("class=\"firstHeading\" lang=\"sl\"><span dir=\"auto\">") - 48);

    QString obcina = nov_odgovor.left(nov_odgovor.indexOf("<"));

    if ( obcina.contains("entjur") ) {
        obcina = obcina + " pri Celju";
    }

    nov_odgovor = nov_odgovor.right(nov_odgovor.length() - nov_odgovor.indexOf("<h2><span class=\"mw-headline\" id=\"Naselja_v_ob.C4.8Dini\">Naselja v ob"));
    nov_odgovor = nov_odgovor.right(nov_odgovor.length() - nov_odgovor.indexOf("</h2>") - 5);
    nov_odgovor = nov_odgovor.left(nov_odgovor.indexOf("<h2>"));
    nov_odgovor = nov_odgovor.left(nov_odgovor.indexOf("."));

    int st_krajev = nov_odgovor.count("</a>");

    QString app_path = QApplication::applicationDirPath();
    QString dbase_path = app_path + "/base.bz";

    {
        QSqlDatabase base = QSqlDatabase::addDatabase("QSQLITE", obcina);
        base.setDatabaseName(dbase_path);
        base.database();
        base.open();
        if ( base.isOpen() != true ) {
            QMessageBox msgbox;
            msgbox.setText("There was a problem with the database");
            msgbox.setInformativeText("Due to unknown reason there was a problem with opening the database.\nThe problem accured during initial opening of the database.");
            msgbox.exec();
        }
        else {
            // the database is open

            QSqlQuery create_table;
            create_table.prepare("CREATE TABLE IF NOT EXISTS kraji ("
                                 "id INTEGER PRIMARY KEY, "
                                 "regija TEXT, "
                                 "obcina TEXT, "
                                 "kraj TEXT, "
                                 "kraj_ga TEXT)");
            create_table.exec();

            QString regija;
            QSqlQuery sql_regija;
            sql_regija.prepare("SELECT * FROM obcine WHERE obcina LIKE '" + obcina + "'");
            sql_regija.exec();
            if ( sql_regija.next() ) {
                regija = sql_regija.value(sql_regija.record().indexOf("regija")).toString();
            }
            else {
                ui->txt_output->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
                ui->txt_output->insertPlainText("Obcini " + obcina + " ne najdem regije!\n");
            }

            for ( int i = 0; i < st_krajev; i++ ) {
                QString kraj = "";
                nov_odgovor = nov_odgovor.right(nov_odgovor.length() - nov_odgovor.indexOf("<a href="));
                nov_odgovor = nov_odgovor.right(nov_odgovor.length() - nov_odgovor.indexOf(">") - 1);
                kraj = nov_odgovor.left(nov_odgovor.indexOf("</a>"));
                nov_odgovor = nov_odgovor.right(nov_odgovor.length() - nov_odgovor.indexOf("</a>"));

                QString kraj_ga = kraj;
                kraj_ga = kraj_ga.replace("\u010c", "C").replace("\u0160", "S").replace("\u017d", "Z");
                kraj_ga = kraj_ga.replace("\u010d", "c").replace("\u0161", "s").replace("\u017e", "z");
                QSqlQuery check;
                check.prepare("SELECT * FROM kraji WHERE kraj LIKE '" + kraj + "'");
                check.exec();
                if ( !check.next() ) {
                    QSqlQuery enter;
                    enter.prepare("INSERT INTO kraji (regija, obcina, kraj, kraj_ga) VALUES (?, ?, ?, ?)");
                    enter.bindValue(0, regija);
                    enter.bindValue(1, obcina);
                    enter.bindValue(2, kraj);
                    enter.bindValue(3, kraj_ga);
                    enter.exec();

                    ui->txt_output->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
                    ui->txt_output->insertPlainText("Vnos kraja: " + kraj + "\n");

                }
                qApp->processEvents();
            }

        }
        base.close();

   }

}

void GlavnoOkno::preveri_izvorno() {

    if ( ui->txt_pot_izvorne->text() == "" ) { // pot je prazna, javi napako
        ui->lbl_izvorna_ok->setText("Vnesi!");
        ui->lbl_izvorna_ok->setStyleSheet("QLabel { color:red; font-weight:bold; }");
        ui->btn_zazeni->setDisabled(true);
    }
    else { // pot je polna, javi OK
        ui->lbl_izvorna_ok->setText("V redu!");
        ui->lbl_izvorna_ok->setStyleSheet("QLabel { color:green; font-weight:normal; }");
        if ( ui->lbl_izhodna_ok->text() == "V redu!" ) {
            ui->btn_zazeni->setDisabled(false);
        }
    }

}

void GlavnoOkno::preveri_izhodno() {

    if ( ui->txt_pot_izhodna->text() == "" ) { // pot je prazna, javi napako
        ui->lbl_izhodna_ok->setText("Vnesi!");
        ui->lbl_izhodna_ok->setStyleSheet("QLabel { color:red; font-weight:bold; }");
        ui->btn_zazeni->setDisabled(true);
    }
    else { // pot je polna, javi OK
        ui->lbl_izhodna_ok->setText("V redu!");
        ui->lbl_izhodna_ok->setStyleSheet("QLabel { color:green; font-weight:normal; }");
        if ( ui->lbl_izvorna_ok->text() == "V redu!" ) {
            ui->btn_zazeni->setDisabled(false);
        }
    }

}
