#ifndef PERSON_H
#define PERSON_H


#include <QJsonObject>
#include <QJsonArray>

struct st_person{
    int bib = 0;
    QString id = "";
    QString group_id = "";
    QString organization_id = "";
    int card_number = 0;
    QString name = "";
    QString surname ="";
    int sex = 0;
    int year = 0;
    QString birth_date = "";
    int qual = 0;
    int start_time = 0;
    int start_group = 0;

    QString national_code = ""; // region russia, 77, 76 ...
    QString world_code = "";
    QString comment ="";
    QString object = "Person";

    bool is_out_of_competition = true;
    bool is_paid = false;
    bool is_personal = false;
    bool is_rented_card = false;

};

/*
"bib": 3346,
"birth_date": null,
"card_number": 666,
"comment": "",
"group_id": null,
"id": "538dcce2-263f-4385-92cf-8f6aae711ae7",
"is_out_of_competition": true,
"is_paid": false,
"is_personal": false,
"is_rented_card": false,
"name": "\u0418\u043b\u044c\u044f",
"national_code": "77",
"object": "Person",
"organization_id": "6f3f498f-4d93-4927-9a47-926a47c12f7c",
"qual": 6,
"sex": 0,
"start_group": 0,
"start_time": 18480000,
"surname": "\u041f\u0435\u0440\u0432\u043e\u0432",
"world_code": "",
"year": 0
*/

QString getQ(int q);

class person
{
public:
    explicit person(int bib);
    person(st_person& dt);
    person(QJsonObject objJson);
    QJsonObject toJson() const;

    explicit person(const person& other) {
        if (this != &other) {
            data.bib = other.data.bib;
            data.card_number = other.data.card_number;
            data.name = other.data.name;
            data.surname = other.data.surname;
            data.sex = other.data.sex;
            data.year = other.data.year;
            data.qual = other.data.qual;
            data.comment = other.data.comment;
            data.object = "Person";
        }
    };

    explicit person(person&& other) noexcept
    {
        data.bib = other.data.bib;
        data.card_number = other.data.card_number;
        data.name = other.data.name;
        data.surname = other.data.surname;
        data.sex = other.data.sex;
        data.year = other.data.year;
        data.qual = other.data.qual;
        data.comment = other.data.comment;
    };

    person& operator=(const person& other) {
        if (this != &other) {
            data.bib = other.data.bib;
            data.card_number = other.data.card_number;
            data.name = other.data.name;
            data.surname = other.data.surname;
            data.sex = other.data.sex;
            data.year = other.data.year;
            data.qual = other.data.qual;
            data.comment = other.data.comment;
            data.object = "Person";
        }
        return *this;
    };

    person& operator=(person&& other) noexcept {
        if (this != &other) {
            data.bib = other.data.bib;
            data.card_number = other.data.card_number;
            data.name = other.data.name;
            data.surname = other.data.surname;
            data.sex = other.data.sex;
            data.year = other.data.year;
            data.qual = other.data.qual;
            data.comment = other.data.comment;
            data.object = "Person";

            //data_ = std::move(other.data_);

            //data.bib = 0;
            //data.card_number = 0;
            // data.name = "";
            // data.surname = "";
            // data.sex = 0;
            // data.year = 0;
            // data.qual = 0;
            // data.comment = "";
        }

        return *this;
    };

    person& operator()(int number) {return *this;};

    bool operator==(const person & obj){return this->data.bib == obj.data.bib; };

    int setCardNum(int cardNum);

    int getBib(){ return data.bib;};
    QString getName(){ return data.name;};
    QString getSurname(){ return data.surname;};
    QString getFullName(){ return data.name + " " + data.surname;};
    int getSex(){ return data.sex;};
    int getCardNumber(){ return data.card_number;};
    int getYear(){ return data.year;};
    int getStartTime(){ return data.start_time;};
    QString getQual(){ return getQ(data.qual);};
    QString getComment(){ return data.comment;};
    QString getOrganizationId(){ return data.organization_id;};
    QString getGroupId(){ return data.group_id;};
    QString getId(){ return data.id;};

    const st_person* getDataPersonBib(){ return &data;};


private:
    st_person data;
};


#endif // PERSON_H
