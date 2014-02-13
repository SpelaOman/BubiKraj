#ifndef GLAVNOOKNO_H
#define GLAVNOOKNO_H

#include <QMainWindow>
#include <QNetworkReply>

namespace Ui {
class GlavnoOkno;
}

class GlavnoOkno : public QMainWindow
{
    Q_OBJECT

public:
    explicit GlavnoOkno(QWidget *parent = 0);
    ~GlavnoOkno();

private slots:
    void on_btn_izhod_clicked();

    void on_btn_izberi_izvorno_datoteko_clicked();
    void on_btn_izberi_izhodno_datoteko_clicked();

    void on_btn_posodobi_regije_obcine_clicked();

    void on_btn_zazeni_clicked();

    void konec_odziva_regije(QNetworkReply *odgovor);
    void konec_odziva_kraji(QNetworkReply *odgovor);

    void preveri_izvorno();
    void preveri_izhodno();

private:
    Ui::GlavnoOkno *ui;
};

#endif // GLAVNOOKNO_H
