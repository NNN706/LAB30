#pragma once
#include <vector>
#include <string>
#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>
#include "BlockSort.h"
#include "Database.h"

using namespace msclr::interop;

namespace SortingApp {

    using namespace System;
    using namespace System::ComponentModel;
    using namespace System::Collections;
    using namespace System::Windows::Forms;
    using namespace System::Data;
    using namespace System::Drawing;
    using namespace System::IO;
    using namespace System::Collections::Generic;
    using namespace System::Runtime::InteropServices;

    public ref class MainForm : public System::Windows::Forms::Form
    {
    public:
        MainForm(void)
        {
            InitializeComponent();

            // Инициализация данных
            currentArray = gcnew System::Collections::Generic::List<int>();
            lastOriginalArray = gcnew System::Collections::Generic::List<int>();
            lastOperationWasSort = false;
            currentUsername = "";
            db = nullptr;

            // Инициализация Microsoft SQL Server базы данных
            try {
                // Создание объекта базы данных
                db = new ::Database();

                if (db->open()) {
                    lblStatus->Text = "База данных Microsoft SQL Server подключена";
                    lblStatus->ForeColor = Color::FromArgb(76, 175, 80);
                    lblDBStatus->Text = "Подключено";
                    lblDBStatus->ForeColor = Color::FromArgb(76, 175, 80);
                    System::Diagnostics::Debug::WriteLine("Database connected successfully!");
                }
                else {
                    lblStatus->Text = "Ошибка подключения к БД Microsoft SQL Server";
                    lblStatus->ForeColor = Color::FromArgb(244, 67, 54);
                    lblDBStatus->Text = "Не подключено";
                    lblDBStatus->ForeColor = Color::FromArgb(244, 67, 54);
                    System::Diagnostics::Debug::WriteLine("Failed to connect to database!");

                    // В режиме отладки показываем сообщение
#ifdef _DEBUG
                    MessageBox::Show(
                        "Не удалось подключиться к базе данных!\n\n" +
                        "Проверьте:\n" +
                        "1. Запущен ли SQL Server\n" +
                        "2. Существует ли база данных SortingDB\n" +
                        "3. Правильность строки подключения\n\n" +
                        "Приложение будет работать в режиме без базы данных.",
                        "Ошибка подключения",
                        MessageBoxButtons::OK,
                        MessageBoxIcon::Warning);
#endif
                }
            }
            catch (System::Exception^ ex) {
                lblStatus->Text = "Ошибка инициализации БД: " + ex->Message;
                lblStatus->ForeColor = Color::FromArgb(244, 67, 54);
                lblDBStatus->Text = "Ошибка";
                lblDBStatus->ForeColor = Color::FromArgb(244, 67, 54);
                System::Diagnostics::Debug::WriteLine("Database initialization error: " + ex->Message);
            }
            catch (...) {
                lblStatus->Text = "Неизвестная ошибка инициализации БД!";
                lblStatus->ForeColor = Color::FromArgb(244, 67, 54);
                lblDBStatus->Text = "Ошибка";
                lblDBStatus->ForeColor = Color::FromArgb(244, 67, 54);
                System::Diagnostics::Debug::WriteLine("Unknown database initialization error!");
            }
        }

        ~MainForm()
        {
            // Закрываем базу данных
            if (db != nullptr) {
                db->close();
                delete db;
                db = nullptr;
            }

            if (components)
            {
                delete components;
            }
        }

    private:
        // Объявления элементов управления Windows Forms
        System::Windows::Forms::GroupBox^ groupAuth;
        System::Windows::Forms::GroupBox^ groupArray;
        System::Windows::Forms::GroupBox^ groupHistory;
        System::Windows::Forms::TextBox^ txtPassword;
        System::Windows::Forms::TextBox^ txtLogin;
        System::Windows::Forms::RichTextBox^ txtArray;
        System::Windows::Forms::Button^ btnRegister;
        System::Windows::Forms::Button^ btnLogin;
        System::Windows::Forms::Button^ btnSave;
        System::Windows::Forms::Button^ btnSort;
        System::Windows::Forms::Button^ btnGenerate;
        System::Windows::Forms::Label^ labelLogin;
        System::Windows::Forms::Button^ btnHelp;
        System::Windows::Forms::Label^ labelPassword;
        System::Windows::Forms::Label^ labelArray;
        System::Windows::Forms::Label^ labelHistory;
        System::Windows::Forms::ListBox^ listHistory;
        System::Windows::Forms::Label^ lblStatus;
        System::Windows::Forms::Label^ lblDBStatus;
        System::Windows::Forms::Panel^ panelHeader;
        System::Windows::Forms::Label^ lblTitle;
        System::Windows::Forms::Panel^ panelFooter;
        System::Windows::Forms::Label^ lblUserInfo;
        System::Windows::Forms::Button^ btnShowOriginal;

        // Данные программы
        System::Collections::Generic::List<int>^ currentArray;
        System::Collections::Generic::List<int>^ lastOriginalArray;
        bool lastOperationWasSort;
        System::String^ currentUsername;
        ::Database* db;

        System::ComponentModel::Container^ components;

        // Вспомогательные методы
        std::string ConvertToString(System::String^ s) {
            if (s == nullptr || s->Length == 0)
                return "";

            const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer();
            std::string result = chars;
            Marshal::FreeHGlobal(System::IntPtr((void*)chars));
            return result;
        }

        String^ ConvertToManagedString(const std::string& s) {
            return gcnew String(s.c_str());
        }

        std::vector<int> ParseTextToVector()
        {
            std::vector<int> out;
            array<String^>^ parts = txtArray->Text->Trim()->Split(' ');
            for each (String ^ part in parts) {
                if (part->Length > 0) {
                    try {
                        out.push_back(Convert::ToInt32(part));
                    }
                    catch (Exception^) {
                        // Игнорируем нечисловые значения
                    }
                }
            }
            return out;
        }

        void UpdateArrayDisplayAndCurrent(const std::vector<int>& vec)
        {
            txtArray->Clear();
            currentArray->Clear();
            for (int v : vec) {
                txtArray->AppendText(v.ToString() + " ");
                currentArray->Add(v);
            }
        }

        void LoadHistory() {
            if (currentUsername == nullptr || currentUsername->Length == 0)
                return;
            if (db == nullptr || !db->isConnected())
                return;

            listHistory->Items->Clear();

            std::string username = ConvertToString(currentUsername);
            auto history = db->getUserArrays(username);

            if (history.empty()) {
                listHistory->Items->Add("История пуста (Microsoft SQL Server)");
                listHistory->Items->Add("Сохраните первый массив!");
                return;
            }

            // Ограничиваем показ 20 последними записями
            int count = 0;
            for (const auto& entry : history) {
                if (count++ >= 20)
                    break;
                listHistory->Items->Add(ConvertToManagedString(entry));
            }

            lblStatus->Text = String::Format("Загружено {0} записей из Microsoft SQL Server", history.size());
        }

        bool CheckDatabaseConnection() {
            if (db == nullptr) return false;
            return db->isConnected();
        }

#pragma region Windows Form Designer generated code
        void InitializeComponent(void)
        {
            this->groupAuth = (gcnew System::Windows::Forms::GroupBox());
            this->labelPassword = (gcnew System::Windows::Forms::Label());
            this->labelLogin = (gcnew System::Windows::Forms::Label());
            this->btnRegister = (gcnew System::Windows::Forms::Button());
            this->btnLogin = (gcnew System::Windows::Forms::Button());
            this->txtPassword = (gcnew System::Windows::Forms::TextBox());
            this->txtLogin = (gcnew System::Windows::Forms::TextBox());
            this->btnHelp = (gcnew System::Windows::Forms::Button());
            this->groupArray = (gcnew System::Windows::Forms::GroupBox());
            this->btnShowOriginal = (gcnew System::Windows::Forms::Button());
            this->labelArray = (gcnew System::Windows::Forms::Label());
            this->btnSave = (gcnew System::Windows::Forms::Button());
            this->btnSort = (gcnew System::Windows::Forms::Button());
            this->btnGenerate = (gcnew System::Windows::Forms::Button());
            this->txtArray = (gcnew System::Windows::Forms::RichTextBox());
            this->groupHistory = (gcnew System::Windows::Forms::GroupBox());
            this->listHistory = (gcnew System::Windows::Forms::ListBox());
            this->labelHistory = (gcnew System::Windows::Forms::Label());
            this->lblStatus = (gcnew System::Windows::Forms::Label());
            this->panelHeader = (gcnew System::Windows::Forms::Panel());
            this->lblDBStatus = (gcnew System::Windows::Forms::Label());
            this->lblTitle = (gcnew System::Windows::Forms::Label());
            this->panelFooter = (gcnew System::Windows::Forms::Panel());
            this->lblUserInfo = (gcnew System::Windows::Forms::Label());
            this->groupAuth->SuspendLayout();
            this->groupArray->SuspendLayout();
            this->groupHistory->SuspendLayout();
            this->panelHeader->SuspendLayout();
            this->panelFooter->SuspendLayout();
            this->SuspendLayout();
            // 
            // groupAuth
            // 
            this->groupAuth->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(248)),
                static_cast<System::Int32>(static_cast<System::Byte>(249)), static_cast<System::Int32>(static_cast<System::Byte>(250)));
            this->groupAuth->Controls->Add(this->labelPassword);
            this->groupAuth->Controls->Add(this->labelLogin);
            this->groupAuth->Controls->Add(this->btnRegister);
            this->groupAuth->Controls->Add(this->btnLogin);
            this->groupAuth->Controls->Add(this->txtPassword);
            this->groupAuth->Controls->Add(this->txtLogin);
            this->groupAuth->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(204)));
            this->groupAuth->Location = System::Drawing::Point(25, 100);
            this->groupAuth->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->groupAuth->Name = L"groupAuth";
            this->groupAuth->Padding = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->groupAuth->Size = System::Drawing::Size(970, 110);
            this->groupAuth->TabIndex = 0;
            this->groupAuth->TabStop = false;
            this->groupAuth->Text = L"Авторизация";
            // 
            // labelPassword
            // 
            this->labelPassword->AutoSize = true;
            this->labelPassword->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->labelPassword->Location = System::Drawing::Point(300, 42);
            this->labelPassword->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
            this->labelPassword->Name = L"labelPassword";
            this->labelPassword->Size = System::Drawing::Size(65, 23);
            this->labelPassword->TabIndex = 5;
            this->labelPassword->Text = L"Пароль:";
            // 
            // labelLogin
            // 
            this->labelLogin->AutoSize = true;
            this->labelLogin->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->labelLogin->Location = System::Drawing::Point(22, 42);
            this->labelLogin->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
            this->labelLogin->Name = L"labelLogin";
            this->labelLogin->Size = System::Drawing::Size(62, 23);
            this->labelLogin->TabIndex = 4;
            this->labelLogin->Text = L"Логин:";
            // 
            // btnRegister
            // 
            this->btnRegister->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(108)),
                static_cast<System::Int32>(static_cast<System::Byte>(117)), static_cast<System::Int32>(static_cast<System::Byte>(125)));
            this->btnRegister->FlatAppearance->BorderSize = 0;
            this->btnRegister->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
            this->btnRegister->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->btnRegister->ForeColor = System::Drawing::Color::White;
            this->btnRegister->Location = System::Drawing::Point(730, 35);
            this->btnRegister->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->btnRegister->Name = L"btnRegister";
            this->btnRegister->Size = System::Drawing::Size(150, 40);
            this->btnRegister->TabIndex = 3;
            this->btnRegister->Text = L"Регистрация";
            this->btnRegister->UseVisualStyleBackColor = false;
            this->btnRegister->Click += gcnew System::EventHandler(this, &MainForm::btnRegister_Click);
            // 
            // btnLogin
            // 
            this->btnLogin->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(33)),
                static_cast<System::Int32>(static_cast<System::Byte>(150)), static_cast<System::Int32>(static_cast<System::Byte>(243)));
            this->btnLogin->FlatAppearance->BorderSize = 0;
            this->btnLogin->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
            this->btnLogin->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->btnLogin->ForeColor = System::Drawing::Color::White;
            this->btnLogin->Location = System::Drawing::Point(565, 35);
            this->btnLogin->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->btnLogin->Name = L"btnLogin";
            this->btnLogin->Size = System::Drawing::Size(150, 40);
            this->btnLogin->TabIndex = 2;
            this->btnLogin->Text = L"Войти";
            this->btnLogin->UseVisualStyleBackColor = false;
            this->btnLogin->Click += gcnew System::EventHandler(this, &MainForm::btnLogin_Click);
            // 
            // txtPassword
            // 
            this->txtPassword->BackColor = System::Drawing::Color::White;
            this->txtPassword->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
            this->txtPassword->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F));
            this->txtPassword->Location = System::Drawing::Point(380, 38);
            this->txtPassword->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->txtPassword->Name = L"txtPassword";
            this->txtPassword->Size = System::Drawing::Size(168, 29);
            this->txtPassword->TabIndex = 1;
            this->txtPassword->Text = L"123";
            this->txtPassword->UseSystemPasswordChar = true;
            // 
            // txtLogin
            // 
            this->txtLogin->BackColor = System::Drawing::Color::White;
            this->txtLogin->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
            this->txtLogin->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F));
            this->txtLogin->Location = System::Drawing::Point(92, 38);
            this->txtLogin->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->txtLogin->Name = L"txtLogin";
            this->txtLogin->Size = System::Drawing::Size(188, 29);
            this->txtLogin->TabIndex = 0;
            this->txtLogin->Text = L"user1";
            // 
            // btnHelp
            // 
            this->btnHelp->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(23)),
                static_cast<System::Int32>(static_cast<System::Byte>(162)), static_cast<System::Int32>(static_cast<System::Byte>(184)));
            this->btnHelp->FlatAppearance->BorderSize = 0;
            this->btnHelp->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
            this->btnHelp->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->btnHelp->ForeColor = System::Drawing::Color::White;
            this->btnHelp->Location = System::Drawing::Point(785, 690);
            this->btnHelp->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->btnHelp->Name = L"btnHelp";
            this->btnHelp->Size = System::Drawing::Size(180, 40);
            this->btnHelp->TabIndex = 6;
            this->btnHelp->Text = L"Справка";
            this->btnHelp->UseVisualStyleBackColor = false;
            this->btnHelp->Click += gcnew System::EventHandler(this, &MainForm::btnHelp_Click);
            // 
            // groupArray
            // 
            this->groupArray->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(248)),
                static_cast<System::Int32>(static_cast<System::Byte>(249)), static_cast<System::Int32>(static_cast<System::Byte>(250)));
            this->groupArray->Controls->Add(this->btnShowOriginal);
            this->groupArray->Controls->Add(this->labelArray);
            this->groupArray->Controls->Add(this->btnSave);
            this->groupArray->Controls->Add(this->btnSort);
            this->groupArray->Controls->Add(this->btnGenerate);
            this->groupArray->Controls->Add(this->txtArray);
            this->groupArray->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(204)));
            this->groupArray->Location = System::Drawing::Point(25, 230);
            this->groupArray->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->groupArray->Name = L"groupArray";
            this->groupArray->Padding = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->groupArray->Size = System::Drawing::Size(970, 270);
            this->groupArray->TabIndex = 1;
            this->groupArray->TabStop = false;
            this->groupArray->Text = L"Работа с массивом";
            // 
            // btnShowOriginal
            // 
            this->btnShowOriginal->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(108)),
                static_cast<System::Int32>(static_cast<System::Byte>(117)), static_cast<System::Int32>(static_cast<System::Byte>(125)));
            this->btnShowOriginal->FlatAppearance->BorderSize = 0;
            this->btnShowOriginal->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
            this->btnShowOriginal->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->btnShowOriginal->ForeColor = System::Drawing::Color::White;
            this->btnShowOriginal->Location = System::Drawing::Point(585, 205);
            this->btnShowOriginal->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->btnShowOriginal->Name = L"btnShowOriginal";
            this->btnShowOriginal->Size = System::Drawing::Size(180, 40);
            this->btnShowOriginal->TabIndex = 7;
            this->btnShowOriginal->Text = L"Исходный вид";
            this->btnShowOriginal->UseVisualStyleBackColor = false;
            this->btnShowOriginal->Click += gcnew System::EventHandler(this, &MainForm::btnShowOriginal_Click);
            // 
            // labelArray
            // 
            this->labelArray->AutoSize = true;
            this->labelArray->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->labelArray->Location = System::Drawing::Point(22, 38);
            this->labelArray->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
            this->labelArray->Name = L"labelArray";
            this->labelArray->Size = System::Drawing::Size(233, 23);
            this->labelArray->TabIndex = 6;
            this->labelArray->Text = L"Массив (через пробел):";
            // 
            // btnSave
            // 
            this->btnSave->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(255)),
                static_cast<System::Int32>(static_cast<System::Byte>(193)), static_cast<System::Int32>(static_cast<System::Byte>(7)));
            this->btnSave->FlatAppearance->BorderSize = 0;
            this->btnSave->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
            this->btnSave->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->btnSave->ForeColor = System::Drawing::Color::Black;
            this->btnSave->Location = System::Drawing::Point(397, 205);
            this->btnSave->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->btnSave->Name = L"btnSave";
            this->btnSave->Size = System::Drawing::Size(180, 40);
            this->btnSave->TabIndex = 5;
            this->btnSave->Text = L"Сохранить в БД";
            this->btnSave->UseVisualStyleBackColor = false;
            this->btnSave->Click += gcnew System::EventHandler(this, &MainForm::btnSave_Click);
            // 
            // btnSort
            // 
            this->btnSort->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(156)),
                static_cast<System::Int32>(static_cast<System::Byte>(39)), static_cast<System::Int32>(static_cast<System::Byte>(176)));
            this->btnSort->FlatAppearance->BorderSize = 0;
            this->btnSort->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
            this->btnSort->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->btnSort->ForeColor = System::Drawing::Color::White;
            this->btnSort->Location = System::Drawing::Point(209, 205);
            this->btnSort->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->btnSort->Name = L"btnSort";
            this->btnSort->Size = System::Drawing::Size(180, 40);
            this->btnSort->TabIndex = 4;
            this->btnSort->Text = L"Сортировать";
            this->btnSort->UseVisualStyleBackColor = false;
            this->btnSort->Click += gcnew System::EventHandler(this, &MainForm::btnSort_Click);
            // 
            // btnGenerate
            // 
            this->btnGenerate->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(76)),
                static_cast<System::Int32>(static_cast<System::Byte>(175)), static_cast<System::Int32>(static_cast<System::Byte>(80)));
            this->btnGenerate->FlatAppearance->BorderSize = 0;
            this->btnGenerate->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
            this->btnGenerate->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->btnGenerate->ForeColor = System::Drawing::Color::White;
            this->btnGenerate->Location = System::Drawing::Point(22, 205);
            this->btnGenerate->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->btnGenerate->Name = L"btnGenerate";
            this->btnGenerate->Size = System::Drawing::Size(180, 40);
            this->btnGenerate->TabIndex = 3;
            this->btnGenerate->Text = L"Сгенерировать";
            this->btnGenerate->UseVisualStyleBackColor = false;
            this->btnGenerate->Click += gcnew System::EventHandler(this, &MainForm::btnGenerate_Click);
            // 
            // txtArray
            // 
            this->txtArray->BackColor = System::Drawing::Color::White;
            this->txtArray->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
            this->txtArray->Font = (gcnew System::Drawing::Font(L"Consolas", 11, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(204)));
            this->txtArray->Location = System::Drawing::Point(22, 69);
            this->txtArray->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->txtArray->Name = L"txtArray";
            this->txtArray->Size = System::Drawing::Size(928, 120);
            this->txtArray->TabIndex = 2;
            this->txtArray->Text = L"5 3 8 1 2 9 4 7 6";
            // 
            // groupHistory
            // 
            this->groupHistory->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(248)),
                static_cast<System::Int32>(static_cast<System::Byte>(249)), static_cast<System::Int32>(static_cast<System::Byte>(250)));
            this->groupHistory->Controls->Add(this->listHistory);
            this->groupHistory->Controls->Add(this->labelHistory);
            this->groupHistory->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(204)));
            this->groupHistory->Location = System::Drawing::Point(25, 520);
            this->groupHistory->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->groupHistory->Name = L"groupHistory";
            this->groupHistory->Padding = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->groupHistory->Size = System::Drawing::Size(970, 160);
            this->groupHistory->TabIndex = 2;
            this->groupHistory->TabStop = false;
            this->groupHistory->Text = L"История сохранений (Microsoft SQL Server)";
            // 
            // listHistory
            // 
            this->listHistory->BackColor = System::Drawing::Color::White;
            this->listHistory->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
            this->listHistory->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F));
            this->listHistory->FormattingEnabled = true;
            this->listHistory->ItemHeight = 23;
            this->listHistory->Location = System::Drawing::Point(22, 62);
            this->listHistory->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->listHistory->Name = L"listHistory";
            this->listHistory->Size = System::Drawing::Size(928, 83);
            this->listHistory->TabIndex = 1;
            // 
            // labelHistory
            // 
            this->labelHistory->AutoSize = true;
            this->labelHistory->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 9.75F, System::Drawing::FontStyle::Bold));
            this->labelHistory->Location = System::Drawing::Point(22, 31);
            this->labelHistory->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
            this->labelHistory->Name = L"labelHistory";
            this->labelHistory->Size = System::Drawing::Size(206, 23);
            this->labelHistory->TabIndex = 0;
            this->labelHistory->Text = L"История сохранений:";
            // 
            // lblStatus
            // 
            this->lblStatus->AutoSize = true;
            this->lblStatus->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(204)));
            this->lblStatus->Location = System::Drawing::Point(30, 710);
            this->lblStatus->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
            this->lblStatus->Name = L"lblStatus";
            this->lblStatus->Size = System::Drawing::Size(152, 23);
            this->lblStatus->TabIndex = 4;
            this->lblStatus->Text = L"Статусная строка";
            // 
            // panelHeader
            // 
            this->panelHeader->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(33)),
                static_cast<System::Int32>(static_cast<System::Byte>(150)), static_cast<System::Int32>(static_cast<System::Byte>(243)));
            this->panelHeader->Controls->Add(this->lblDBStatus);
            this->panelHeader->Controls->Add(this->lblTitle);
            this->panelHeader->Dock = System::Windows::Forms::DockStyle::Top;
            this->panelHeader->Location = System::Drawing::Point(0, 0);
            this->panelHeader->Name = L"panelHeader";
            this->panelHeader->Size = System::Drawing::Size(1026, 70);
            this->panelHeader->TabIndex = 7;
            // 
            // lblDBStatus
            // 
            this->lblDBStatus->AutoSize = true;
            this->lblDBStatus->Font = (gcnew System::Drawing::Font(L"Segoe UI Semibold", 10.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(204)));
            this->lblDBStatus->ForeColor = System::Drawing::Color::White;
            this->lblDBStatus->Location = System::Drawing::Point(850, 27);
            this->lblDBStatus->Name = L"lblDBStatus";
            this->lblDBStatus->Size = System::Drawing::Size(126, 23);
            this->lblDBStatus->TabIndex = 1;
            this->lblDBStatus->Text = L"Состояние БД";
            // 
            // lblTitle
            // 
            this->lblTitle->AutoSize = true;
            this->lblTitle->Font = (gcnew System::Drawing::Font(L"Segoe UI", 16.2F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(204)));
            this->lblTitle->ForeColor = System::Drawing::Color::White;
            this->lblTitle->Location = System::Drawing::Point(20, 20);
            this->lblTitle->Name = L"lblTitle";
            this->lblTitle->Size = System::Drawing::Size(625, 38);
            this->lblTitle->TabIndex = 0;
            this->lblTitle->Text = L"Сортировщик массивов с Microsoft SQL Server";
            // 
            // panelFooter
            // 
            this->panelFooter->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(52)),
                static_cast<System::Int32>(static_cast<System::Byte>(73)), static_cast<System::Int32>(static_cast<System::Byte>(94)));
            this->panelFooter->Controls->Add(this->lblUserInfo);
            this->panelFooter->Dock = System::Windows::Forms::DockStyle::Bottom;
            this->panelFooter->Location = System::Drawing::Point(0, 750);
            this->panelFooter->Name = L"panelFooter";
            this->panelFooter->Size = System::Drawing::Size(1026, 50);
            this->panelFooter->TabIndex = 8;
            // 
            // lblUserInfo
            // 
            this->lblUserInfo->AutoSize = true;
            this->lblUserInfo->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(204)));
            this->lblUserInfo->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(189)),
                static_cast<System::Int32>(static_cast<System::Byte>(195)), static_cast<System::Int32>(static_cast<System::Byte>(199)));
            this->lblUserInfo->Location = System::Drawing::Point(20, 15);
            this->lblUserInfo->Name = L"lblUserInfo";
            this->lblUserInfo->Size = System::Drawing::Size(265, 23);
            this->lblUserInfo->TabIndex = 0;
            this->lblUserInfo->Text = L"Пользователь: не авторизован";
            // 
            // MainForm
            // 
            this->AutoScaleDimensions = System::Drawing::SizeF(8, 20);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(233)),
                static_cast<System::Int32>(static_cast<System::Byte>(236)), static_cast<System::Int32>(static_cast<System::Byte>(239)));
            this->ClientSize = System::Drawing::Size(1026, 800);
            this->Controls->Add(this->panelFooter);
            this->Controls->Add(this->panelHeader);
            this->Controls->Add(this->lblStatus);
            this->Controls->Add(this->btnHelp);
            this->Controls->Add(this->groupHistory);
            this->Controls->Add(this->groupArray);
            this->Controls->Add(this->groupAuth);
            this->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
                static_cast<System::Byte>(204)));
            this->Margin = System::Windows::Forms::Padding(4, 5, 4, 5);
            this->MaximumSize = System::Drawing::Size(1048, 850);
            this->MinimumSize = System::Drawing::Size(1048, 850);
            this->Name = L"MainForm";
            this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
            this->Text = L"Сортировщик массивов";
            this->groupAuth->ResumeLayout(false);
            this->groupAuth->PerformLayout();
            this->groupArray->ResumeLayout(false);
            this->groupArray->PerformLayout();
            this->groupHistory->ResumeLayout(false);
            this->groupHistory->PerformLayout();
            this->panelHeader->ResumeLayout(false);
            this->panelHeader->PerformLayout();
            this->panelFooter->ResumeLayout(false);
            this->panelFooter->PerformLayout();
            this->ResumeLayout(false);
            this->PerformLayout();
        }
#pragma endregion

    private:
        System::Void btnLogin_Click(System::Object^ sender, System::EventArgs^ e) {
            String^ login = txtLogin->Text;
            String^ password = txtPassword->Text;

            if (login->Length == 0 || password->Length == 0) {
                MessageBox::Show("Введите логин и пароль!", "Ошибка входа",
                    MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            if (db == nullptr || !db->isConnected()) {
                MessageBox::Show("База данных не подключена!", "Ошибка",
                    MessageBoxButtons::OK, MessageBoxIcon::Error);
                return;
            }

            std::string username = ConvertToString(login);
            std::string pass = ConvertToString(password);

            int userId = db->getUserId(username);
            if (userId != -1) {
                // Пользователь найден, проверяем пароль
                if (db->checkUser(username, pass)) {
                    currentUsername = login;
                    lblStatus->Text = "Вход выполнен: " + login + " (Microsoft SQL Server)";
                    lblStatus->ForeColor = Color::FromArgb(76, 175, 80);

                    // Обновляем информацию в футере
                    lblUserInfo->Text = "Пользователь: " + login;
                    lblUserInfo->ForeColor = Color::FromArgb(189, 195, 199);

                    // Блокируем поля авторизации
                    this->txtLogin->Enabled = false;
                    this->txtPassword->Enabled = false;
                    this->btnLogin->Enabled = false;
                    this->btnRegister->Enabled = false;

                    LoadHistory();
                }
                else {
                    MessageBox::Show("Неверный пароль.", "Ошибка входа",
                        MessageBoxButtons::OK, MessageBoxIcon::Error);
                }
            }
            else {
                // Пользователь не найден, предлагаем зарегистрироваться
                if (MessageBox::Show("Пользователь не найден. Зарегистрироваться?", "Вход",
                    MessageBoxButtons::YesNo, MessageBoxIcon::Question) == Windows::Forms::DialogResult::Yes) {
                    if (db->addUser(username, pass)) {
                        currentUsername = login;
                        lblStatus->Text = "Зарегистрирован и вошел: " + login;
                        lblStatus->ForeColor = Color::FromArgb(76, 175, 80);

                        // Обновляем информацию в футере
                        lblUserInfo->Text = "Пользователь: " + login + " (новый)";
                        lblUserInfo->ForeColor = Color::FromArgb(189, 195, 199);

                        this->txtLogin->Enabled = false;
                        this->txtPassword->Enabled = false;
                        this->btnLogin->Enabled = false;
                        this->btnRegister->Enabled = false;
                        LoadHistory();
                    }
                    else {
                        MessageBox::Show("Ошибка регистрации! Попробуйте другой логин.", "Ошибка",
                            MessageBoxButtons::OK, MessageBoxIcon::Error);
                    }
                }
            }
        }

    private:
        System::Void btnRegister_Click(System::Object^ sender, System::EventArgs^ e) {
            String^ login = txtLogin->Text;
            String^ password = txtPassword->Text;

            if (login->Length == 0 || password->Length == 0) {
                MessageBox::Show("Введите логин и пароль!", "Ошибка регистрации",
                    MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            if (db == nullptr || !db->isConnected()) {
                MessageBox::Show("База данных не подключена!", "Ошибка",
                    MessageBoxButtons::OK, MessageBoxIcon::Error);
                return;
            }

            std::string username = ConvertToString(login);
            std::string pass = ConvertToString(password);

            int userId = db->getUserId(username);
            if (userId != -1) {
                MessageBox::Show("Логин уже занят. Выберите другой логин или войдите.", "Ошибка регистрации",
                    MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            if (db->addUser(username, pass)) {
                MessageBox::Show("Пользователь '" + login + "' успешно зарегистрирован в Microsoft SQL Server!\n\n" +
                    "Теперь вы можете войти в систему.",
                    "Регистрация успешна",
                    MessageBoxButtons::OK, MessageBoxIcon::Information);

                lblStatus->Text = "Зарегистрирован новый пользователь: " + login;
                lblStatus->ForeColor = Color::FromArgb(255, 193, 7);
            }
            else {
                MessageBox::Show("Ошибка регистрации! Попробуйте другой логин.",
                    "Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Error);
            }
        }

    private:
        System::Void btnHelp_Click(System::Object^ sender, System::EventArgs^ e) {
            String^ help = "Использование приложения:\n\n";
            help += "1) Введите логин и пароль в соответствующие поля.\n";
            help += "2) Нажмите 'Войти' для авторизации.\n";
            help += "3) Если учетной записи нет, вы можете зарегистрироваться.\n";
            help += "4) Работа с массивом:\n";
            help += "   - 'Сгенерировать' - создает случайный массив\n";
            help += "   - 'Сортировать' - выполняет блочную сортировку\n";
            help += "   - 'Исходный вид' - показывает исходный массив\n";
            help += "   - 'Сохранить в БД' - сохраняет массив в базу данных\n";
            help += "5) В истории отображаются все сохраненные массивы.\n\n";
            help += "Сортировка: используется блочная сортировка (Block Sort)\n";
            help += "База данных: Microsoft SQL Server";

            MessageBox::Show(help, "Справка", MessageBoxButtons::OK, MessageBoxIcon::Information);
        }

    private:
        System::Void btnGenerate_Click(System::Object^ sender, System::EventArgs^ e) {
            Random^ rand = gcnew Random();
            int count = rand->Next(5, 25);

            std::vector<int> vec;
            vec.reserve(count);
            for (int i = 0; i < count; ++i)
                vec.push_back(rand->Next(-100, 100));

            UpdateArrayDisplayAndCurrent(vec);

            lastOriginalArray->Clear();
            lastOperationWasSort = false;

            lblStatus->Text = String::Format("Сгенерировано {0} случайных чисел (-100..100)", count);
            lblStatus->ForeColor = Color::FromArgb(33, 150, 243);
        }

    private:
        System::Void btnSort_Click(System::Object^ sender, System::EventArgs^ e) {
            std::vector<int> parsed = ParseTextToVector();
            if (parsed.empty()) {
                MessageBox::Show("Введите массив чисел!\n\nПример: 5 3 8 1 2",
                    "Ошибка сортировки",
                    MessageBoxButtons::OK, MessageBoxIcon::Error);
                return;
            }

            lastOriginalArray->Clear();
            for (int v : parsed)
                lastOriginalArray->Add(v);

            std::vector<int> sortedVec = parsed;
            sort::BlockSorter::sort(sortedVec);

            UpdateArrayDisplayAndCurrent(sortedVec);
            lastOperationWasSort = true;

            lblStatus->Text = String::Format("Отсортировано {0} элементов (блочная сортировка)", parsed.size());
            lblStatus->ForeColor = Color::FromArgb(156, 39, 176);
        }

    private:
        System::Void btnShowOriginal_Click(System::Object^ sender, System::EventArgs^ e) {
            if (lastOriginalArray->Count > 0) {
                std::vector<int> originalVec;
                for each (int v in lastOriginalArray)
                    originalVec.push_back(v);

                UpdateArrayDisplayAndCurrent(originalVec);
                lblStatus->Text = "Показан исходный массив";
                lblStatus->ForeColor = Color::FromArgb(108, 117, 125);
            }
            else {
                MessageBox::Show("Исходный массив не сохранен. Сначала выполните сортировку.",
                    "Информация", MessageBoxButtons::OK, MessageBoxIcon::Information);
            }
        }

    private:
        System::Void btnSave_Click(System::Object^ sender, System::EventArgs^ e) {
            if (currentUsername == nullptr || currentUsername->Length == 0) {
                MessageBox::Show("Сначала войдите в систему!", "Ошибка сохранения",
                    MessageBoxButtons::OK, MessageBoxIcon::Warning);
                return;
            }

            if (db == nullptr || !db->isConnected()) {
                MessageBox::Show("База данных не подключена!", "Ошибка",
                    MessageBoxButtons::OK, MessageBoxIcon::Error);
                return;
            }

            std::vector<int> original, sorted;

            if (lastOperationWasSort && lastOriginalArray->Count > 0) {
                for each (int v in lastOriginalArray)
                    original.push_back(v);
                for each (int v in currentArray)
                    sorted.push_back(v);
            }
            else {
                original = ParseTextToVector();
                if (original.empty()) {
                    MessageBox::Show("Нет данных для сохранения!\nСгенерируйте или введите массив.",
                        "Ошибка сохранения",
                        MessageBoxButtons::OK, MessageBoxIcon::Warning);
                    return;
                }
                sorted = original;
                sort::BlockSorter::sort(sorted);
            }

            std::string username = ConvertToString(currentUsername);

            // Показать информацию для отладки
            String^ debugInfo = "Информация для отладки:\n";
            debugInfo += "Логин: " + currentUsername + "\n";
            debugInfo += "Размер оригинального массива: " + original.size() + "\n";
            debugInfo += "Размер отсортированного массива: " + sorted.size() + "\n";
            debugInfo += "Оригинальный массив: ";
            for (size_t i = 0; i < min(original.size(), (size_t)5); ++i) {
                debugInfo += original[i].ToString() + " ";
            }
            if (original.size() > 5) debugInfo += "...";
            debugInfo += "\n";

            // Для отладки можно вывести в консоль
            System::Diagnostics::Debug::WriteLine(debugInfo);

            if (db->saveArray(username, original, sorted)) {
                lblStatus->Text = "Массив успешно сохранен в Microsoft SQL Server";
                lblStatus->ForeColor = Color::FromArgb(76, 175, 80);

                MessageBox::Show("Данные сохранены в Microsoft SQL Server!\n\n" +
                    "Оригинальный массив: " + original.size() + " элементов\n" +
                    "Отсортированный массив: " + sorted.size() + " элементов",
                    "Сохранение успешно",
                    MessageBoxButtons::OK, MessageBoxIcon::Information);

                LoadHistory();

                // Сбрасываем состояние
                currentArray->Clear();
                for (int num : original) {
                    currentArray->Add(num);
                }

                lastOriginalArray->Clear();
                lastOperationWasSort = false;
            }
            else {
                // Более подробное сообщение об ошибке
                String^ errorMessage = "Ошибка сохранения в БД Microsoft SQL Server!\n\n";
                errorMessage += "Возможные причины:\n";
                errorMessage += "1. Нет подключения к базе данных\n";
                errorMessage += "2. Ошибка в SQL запросе\n";
                errorMessage += "3. Проблемы с кодировкой данных\n\n";
                errorMessage += "Проверьте:\n";
                errorMessage += "- Подключение к SQL Server\n";
                errorMessage += "- Существование таблиц Users и SortingHistory\n";

                MessageBox::Show(errorMessage, "Ошибка сохранения",
                    MessageBoxButtons::OK, MessageBoxIcon::Error);

                lblStatus->Text = "Ошибка сохранения! Проверьте подключение к БД";
                lblStatus->ForeColor = Color::FromArgb(244, 67, 54);
            }
        }
    };
}