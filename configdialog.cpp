#include "configdialog.h"
#include "qt2sdl2.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QKeyEvent>

ControllerTab::ControllerTab(unsigned int controller)
{
    QGridLayout *layout = new QGridLayout(this);
    QLabel *profileLabel = new QLabel("Profile", this);
    layout->addWidget(profileLabel, 0, 0);

    profileSelect = new QComboBox(this);
    profileSelect->addItems(settings->childGroups());
    profileSelect->removeItem(profileSelect->findText("Auto-Gamepad"));
    profileSelect->removeItem(profileSelect->findText("Auto-Keyboard"));
    profileSelect->insertItem(0, "Auto");
    profileSelect->setCurrentText(controllerSettings->value("Controller" + QString::number(controller) + "/Profile").toString());
    connect(profileSelect, &QComboBox::currentTextChanged, [=](QString text) {
        controllerSettings->setValue("Controller" + QString::number(controller) + "/Profile", text);
    });
    layout->addWidget(profileSelect, 0, 1);

    QLabel *gamepadLabel = new QLabel("Gamepad", this);
    layout->addWidget(gamepadLabel, 1, 0);

    gamepadSelect = new QComboBox(this);
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i))
            gamepadSelect->addItem(QString::number(i) + ":" + SDL_GameControllerNameForIndex(i));
        else
            gamepadSelect->addItem(QString::number(i) + ":" + SDL_JoystickNameForIndex(i));
    }
    gamepadSelect->insertItem(0, "Auto");
    gamepadSelect->addItem("Keyboard");
    gamepadSelect->addItem("None");
    gamepadSelect->setCurrentText(controllerSettings->value("Controller" + QString::number(controller) + "/Gamepad").toString());
    connect(gamepadSelect, &QComboBox::currentTextChanged, [=](QString text) {
        controllerSettings->setValue("Controller" + QString::number(controller) + "/Gamepad", text);
    });
    layout->addWidget(gamepadSelect, 1, 1);

    QLabel *pakLabel = new QLabel("Pak", this);
    layout->addWidget(pakLabel, 2, 0);

    pakSelect = new QComboBox(this);
    pakSelect->addItem("Memory");
    pakSelect->addItem("Rumble");
    pakSelect->addItem("Transfer");
    pakSelect->addItem("None");
    pakSelect->setCurrentText(controllerSettings->value("Controller" + QString::number(controller) + "/Pak").toString());
    connect(pakSelect, &QComboBox::currentTextChanged, [=](QString text) {
        controllerSettings->setValue("Controller" + QString::number(controller) + "/Pak", text);
    });
    layout->addWidget(pakSelect, 2, 1);

    setLayout(layout);
}

void ProfileTab::setComboBox(QComboBox* box, ControllerTab **_controllerTabs)
{
    for (int i = 1; i < 5; ++i) {
        QString current = controllerSettings->value("Controller" + QString::number(i) + "/Profile").toString();
        _controllerTabs[i-1]->profileSelect->clear();
        _controllerTabs[i-1]->profileSelect->addItems(settings->childGroups());
        _controllerTabs[i-1]->profileSelect->removeItem(_controllerTabs[i-1]->profileSelect->findText("Auto-Gamepad"));
        _controllerTabs[i-1]->profileSelect->removeItem(_controllerTabs[i-1]->profileSelect->findText("Auto-Keyboard"));
        _controllerTabs[i-1]->profileSelect->insertItem(0, "Auto");
        _controllerTabs[i-1]->profileSelect->setCurrentText(current);
    }
    box->clear();
    box->addItems(settings->childGroups());
    box->removeItem(box->findText("Auto-Gamepad"));
    box->removeItem(box->findText("Auto-Keyboard"));
}

int ProfileTab::checkNotRunning()
{
    if (emu_running) {
        QMessageBox msgBox;
        msgBox.setText("Stop game before editing profiles.");
        msgBox.exec();
        return 0;
    }
    return 1;
}

ProfileTab::ProfileTab(ControllerTab **_controllerTabs)
{
    QGridLayout *layout = new QGridLayout(this);
    QComboBox *profileSelect = new QComboBox(this);
    setComboBox(profileSelect, _controllerTabs);
    QPushButton *buttonNewKeyboard = new QPushButton("New Profile (Keyboard)", this);
    connect(buttonNewKeyboard, &QPushButton::released, [=]() {
        if (checkNotRunning()) {
            ProfileEditor editor("Auto-Keyboard");
            editor.exec();
            setComboBox(profileSelect, _controllerTabs);
        }
    });
    QPushButton *buttonNewGamepad = new QPushButton("New Profile (Gamepad)", this);
    connect(buttonNewGamepad, &QPushButton::released, [=]() {
        if (checkNotRunning()) {
            ProfileEditor editor("Auto-Gamepad");
            editor.exec();
            setComboBox(profileSelect, _controllerTabs);
        }
    });
    QPushButton *buttonEdit = new QPushButton("Edit Profile", this);
    connect(buttonEdit, &QPushButton::released, [=]() {
        if (!profileSelect->currentText().isEmpty() && checkNotRunning()) {
            ProfileEditor editor(profileSelect->currentText());
            editor.exec();
        }
    });

    QPushButton *buttonDelete = new QPushButton("Delete Profile", this);
    connect(buttonDelete, &QPushButton::released, [=]() {
        if (!profileSelect->currentText().isEmpty()) {
            settings->remove(profileSelect->currentText());
            setComboBox(profileSelect, _controllerTabs);
        }
    });

    layout->addWidget(profileSelect, 1, 0);
    layout->addWidget(buttonNewKeyboard, 0, 1);
    layout->addWidget(buttonNewGamepad, 0, 2);
    layout->addWidget(buttonEdit, 1, 1);
    layout->addWidget(buttonDelete, 1, 2);
    setLayout(layout);
}

CustomButton::CustomButton(QString section, QString setting, QWidget* parent)
{
    ProfileEditor* editor = (ProfileEditor*) parent;
    item = setting;
    QString direction;
    QList<int> value = settings->value(section + "/" + item).value<QList<int> >();
    switch (value.at(1)) {
        case 0/*Keyboard*/:
            type = 0;
            key = (SDL_Scancode)value.at(0);
            this->setText(SDL_GetScancodeName(key));
            break;
        case 1/*Button*/:
            type = 1;
            button = (SDL_GameControllerButton)value.at(0);
            this->setText(SDL_GameControllerGetStringForButton((SDL_GameControllerButton)value.at(0)));
            break;
        case 2/*Axis*/:
            type = 2;
            axis = (SDL_GameControllerAxis)value.at(0);
            axisValue = value.at(2);
            direction = axisValue > 0 ? " +" : " -";
            this->setText(SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)value.at(0)) + direction);
            break;
        case 3/*Joystick Hat*/:
            type = 3;
            joystick_hat = value.at(0);
            axisValue = value.at(2);
            this->setText("Hat " + QString::number(joystick_hat) + " " + QString::number(axisValue));
            break;
        case 4/*Joystick Button*/:
            type = 4;
            joystick_button = value.at(0);
            this->setText("Button " + QString::number(joystick_button));
            break;
        case 5/*Joystick Axis*/:
            type = 5;
            joystick_axis = value.at(0);
            axisValue = value.at(2);
            direction = axisValue > 0 ? " +" : " -";
            this->setText("Axis " + QString::number(joystick_axis) + direction);
           break;
    }
    connect(this, &QPushButton::released, [=]{
        editor->acceptInput(this);
    });
}

void ProfileEditor::keyReleaseEvent(QKeyEvent *event)
{
    int modValue = QT2SDL2MOD(event->modifiers());
    int keyValue = QT2SDL2(event->key());
    SDL_Scancode value = (SDL_Scancode)((modValue << 16) + keyValue);
    if (activeButton != nullptr) {
        killTimer(timer);
        activeButton->type = 0;
        activeButton->key = value;
        activeButton->setText(SDL_GetScancodeName(activeButton->key));
        activeButton = nullptr;
        for (int i = 0; i < buttonList.size(); ++i) {
            buttonList.at(i)->setDisabled(0);
        }
        for (int i = 0; i < checkBoxList.size(); ++i) {
            checkBoxList.at(i)->setDisabled(0);
        }
        for (int i = 0; i < sliderList.size(); ++i) {
            sliderList.at(i)->setDisabled(0);
        }
        for (int i = 0; i < pushButtonList.size(); ++i) {
            pushButtonList.at(i)->setDisabled(0);
        }
        for (int i = 0; i < lineEditList.size(); ++i) {
            if (!editingProfile) {
                lineEditList.at(i)->setDisabled(0);
            }
        }
    }
}

void ProfileEditor::timerEvent(QTimerEvent *)
{
    int i;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_CONTROLLERBUTTONDOWN:
                killTimer(timer);
                activeButton->type = 1;
                activeButton->button = (SDL_GameControllerButton)e.cbutton.button;
                activeButton->setText(SDL_GameControllerGetStringForButton(activeButton->button));
                activeButton = nullptr;
                for (i = 0; i < buttonList.size(); ++i)
                    buttonList.at(i)->setDisabled(0);
                for (i = 0; i < checkBoxList.size(); ++i)
                    checkBoxList.at(i)->setDisabled(0);
                for (i = 0; i < sliderList.size(); ++i)
                    sliderList.at(i)->setDisabled(0);
                for (i = 0; i < pushButtonList.size(); ++i)
                    pushButtonList.at(i)->setDisabled(0);
                for (i = 0; i < lineEditList.size(); ++i)
                    if (!editingProfile)
                        lineEditList.at(i)->setDisabled(0);
                return;
            case SDL_CONTROLLERAXISMOTION:
                if (abs(e.caxis.value) > 16384) {
                    killTimer(timer);
                    activeButton->type = 2;
                    activeButton->axis = (SDL_GameControllerAxis)e.caxis.axis;
                    activeButton->axisValue = e.caxis.value > 0 ? 1 : -1;
                    QString direction = activeButton->axisValue > 0 ? " +" : " -";
                    activeButton->setText(SDL_GameControllerGetStringForAxis(activeButton->axis) + direction);
                    activeButton = nullptr;
                    for (i = 0; i < buttonList.size(); ++i)
                        buttonList.at(i)->setDisabled(0);
                    for (i = 0; i < checkBoxList.size(); ++i)
                        checkBoxList.at(i)->setDisabled(0);
                    for (i = 0; i < sliderList.size(); ++i)
                        sliderList.at(i)->setDisabled(0);
                    for (i = 0; i < pushButtonList.size(); ++i)
                        pushButtonList.at(i)->setDisabled(0);
                    for (i = 0; i < lineEditList.size(); ++i)
                        if (!editingProfile)
                            lineEditList.at(i)->setDisabled(0);
                    return;
                }
                break;
            case SDL_JOYHATMOTION:
                killTimer(timer);
                activeButton->type = 3;
                activeButton->joystick_hat = e.jhat.hat;
                activeButton->axisValue = e.jhat.value;
                activeButton->setText("Hat " + QString::number(activeButton->joystick_hat) + " " + QString::number(activeButton->axisValue));
                activeButton = nullptr;
                for (i = 0; i < buttonList.size(); ++i)
                    buttonList.at(i)->setDisabled(0);
                for (i = 0; i < checkBoxList.size(); ++i)
                    checkBoxList.at(i)->setDisabled(0);
                for (i = 0; i < sliderList.size(); ++i)
                    sliderList.at(i)->setDisabled(0);
                for (i = 0; i < pushButtonList.size(); ++i)
                    pushButtonList.at(i)->setDisabled(0);
                for (i = 0; i < lineEditList.size(); ++i)
                    if (!editingProfile)
                        lineEditList.at(i)->setDisabled(0);
                return;
            case SDL_JOYBUTTONDOWN:
                killTimer(timer);
                activeButton->type = 4;
                activeButton->joystick_button = e.jbutton.button;
                activeButton->setText("Button " + QString::number(activeButton->joystick_button));
                activeButton = nullptr;
                for (i = 0; i < buttonList.size(); ++i)
                    buttonList.at(i)->setDisabled(0);
                for (i = 0; i < checkBoxList.size(); ++i)
                    checkBoxList.at(i)->setDisabled(0);
                for (i = 0; i < sliderList.size(); ++i)
                    sliderList.at(i)->setDisabled(0);
                for (i = 0; i < pushButtonList.size(); ++i)
                    pushButtonList.at(i)->setDisabled(0);
                for (i = 0; i < lineEditList.size(); ++i)
                    if (!editingProfile)
                        lineEditList.at(i)->setDisabled(0);
                return;
            case SDL_JOYAXISMOTION:
                if (abs(e.jaxis.value) > 16384) {
                    killTimer(timer);
                    activeButton->type = 5;
                    activeButton->joystick_axis = e.jaxis.axis;
                    activeButton->axisValue = e.jaxis.value > 0 ? 1 : -1;
                    QString direction = activeButton->axisValue > 0 ? " +" : " -";
                    activeButton->setText("Axis " + QString::number(activeButton->joystick_axis) + direction);
                    activeButton = nullptr;
                    for (i = 0; i < buttonList.size(); ++i)
                        buttonList.at(i)->setDisabled(0);
                    for (i = 0; i < checkBoxList.size(); ++i)
                        checkBoxList.at(i)->setDisabled(0);
                    for (i = 0; i < sliderList.size(); ++i)
                        sliderList.at(i)->setDisabled(0);
                    for (i = 0; i < pushButtonList.size(); ++i)
                        pushButtonList.at(i)->setDisabled(0);
                    for (i = 0; i < lineEditList.size(); ++i)
                        if (!editingProfile)
                            lineEditList.at(i)->setDisabled(0);
                    return;
                }
                break;
        }
    }

    if (buttonTimer == 0) {
        killTimer(timer);
        activeButton->setText(activeButton->origText);
        activeButton = nullptr;
        for (i = 0; i < buttonList.size(); ++i) {
            buttonList.at(i)->setDisabled(0);
        }
        for (i = 0; i < checkBoxList.size(); ++i) {
            checkBoxList.at(i)->setDisabled(0);
        }
        for (i = 0; i < sliderList.size(); ++i) {
            sliderList.at(i)->setDisabled(0);
        }
        for (i = 0; i < pushButtonList.size(); ++i) {
            pushButtonList.at(i)->setDisabled(0);
        }
        for (i = 0; i < lineEditList.size(); ++i) {
            if (!editingProfile) {
            lineEditList.at(i)->setDisabled(0);
            }
        }
        return;
    }
    --buttonTimer;
    activeButton->setText(QString::number(ceil(buttonTimer/10.0)));
}

void ProfileEditor::acceptInput(CustomButton* button)
{
    activeButton = button;
    for (int i = 0; i < buttonList.size(); ++i) {
        buttonList.at(i)->setDisabled(1);
    }
    for (int i = 0; i < checkBoxList.size(); ++i) {
        checkBoxList.at(i)->setDisabled(1);
    }
    for (int i = 0; i < sliderList.size(); ++i) {
        sliderList.at(i)->setDisabled(1);
    }
    for (int i = 0; i < pushButtonList.size(); ++i) {
        pushButtonList.at(i)->setDisabled(1);
    }
    for (int i = 0; i < lineEditList.size(); ++i) {
        lineEditList.at(i)->setDisabled(1);
    }
    buttonTimer = 50;
    activeButton->origText = activeButton->text();
    activeButton->setText(QString::number(buttonTimer/10));
    SDL_Event e;
    while (SDL_PollEvent(&e)) {}
    timer = startTimer(100);
}

ProfileEditor::~ProfileEditor()
{
    for (int i = 0; i < 4; ++i) {
        if (gamepad[i]) {
            SDL_GameControllerClose(gamepad[i]);
            gamepad[i] = NULL;
        }
        else if (joystick[i]) {
            SDL_JoystickClose(joystick[i]);
            joystick[i] = NULL;
        }
    }
}

ProfileEditor::ProfileEditor(QString profile)
{
    memset(gamepad, 0, sizeof(SDL_GameController*) * 4);
    memset(joystick, 0, sizeof(SDL_Joystick*) * 4);
    for (int i = 0; i < 4; ++i) {
        if (SDL_IsGameController(i))
            gamepad[i] = SDL_GameControllerOpen(i);
        else
            joystick[i] = SDL_JoystickOpen(i);
    }

    activeButton = nullptr;
    QString section = profile;
    QLineEdit *profileName = new QLineEdit(this);
    if (profile == "Auto-Keyboard" || profile == "Auto-Gamepad") {
        editingProfile = false;
        profileName->setDisabled(0);
        profile = "";
    }
    else {
        editingProfile = true;
        profileName->setDisabled(1);
    }

    QString toggleToolTip = "When enabled, press the key to enter this mode and press the key once more to leave it.\n"
                            "When disabled, you must hold the key to stay in this mode.";
    QString turboControllerButtonToolTip = "Allow turbo?";
    QString turboRateSliderToolTip = "If turbo mode seemingly does nothing, try increasing the number of wait-frames.\n"
                                     "Turbo inputs are held during the first half of wait-frames and released during the remainder.";
    QString absoluteXYAxisSensitivitySliderToolTip = "Allows for precise control stick movements, using a keyboard.";

    QGridLayout *layout = new QGridLayout(this);
    QLabel *profileNameLabel = new QLabel("Profile Name", this);
    profileName->setText(profile);
    lineEditList.append(profileName);
    layout->addWidget(profileNameLabel, 0, 4);
    layout->addWidget(profileName, 0, 5);

    QFrame* lineH = new QFrame(this);
    lineH->setFrameShape(QFrame::HLine);
    lineH->setFrameShadow(QFrame::Sunken);
    layout->addWidget(lineH, 1, 0, 1, 11);

    QLabel *buttonLabelA = new QLabel("A", this);
    buttonLabelA->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushA = new CustomButton(section, "A", this);
    buttonList.append(buttonPushA);
    layout->addWidget(buttonLabelA, 2, 0);
    layout->addWidget(buttonPushA, 2, 1);

    QLabel *buttonLabelB = new QLabel("B", this);
    buttonLabelB->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushB = new CustomButton(section, "B", this);
    buttonList.append(buttonPushB);
    layout->addWidget(buttonLabelB, 3, 0);
    layout->addWidget(buttonPushB, 3, 1);

    QLabel *buttonLabelZ = new QLabel("Z", this);
    buttonLabelZ->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushZ = new CustomButton(section, "Z", this);
    buttonList.append(buttonPushZ);
    layout->addWidget(buttonLabelZ, 4, 0);
    layout->addWidget(buttonPushZ, 4, 1);

    QLabel *buttonLabelStart = new QLabel("Start", this);
    buttonLabelStart->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushStart = new CustomButton(section, "Start", this);
    buttonList.append(buttonPushStart);
    layout->addWidget(buttonLabelStart, 5, 0);
    layout->addWidget(buttonPushStart, 5, 1);

    QLabel *buttonLabelLTrigger = new QLabel("Left Trigger", this);
    buttonLabelLTrigger->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushLTrigger = new CustomButton(section, "L", this);
    buttonList.append(buttonPushLTrigger);
    layout->addWidget(buttonLabelLTrigger, 6, 0);
    layout->addWidget(buttonPushLTrigger, 6, 1);

    QLabel *buttonLabelRTrigger = new QLabel("Right Trigger", this);
    buttonLabelRTrigger->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushRTrigger = new CustomButton(section, "R", this);
    buttonList.append(buttonPushRTrigger);
    layout->addWidget(buttonLabelRTrigger, 7, 0);
    layout->addWidget(buttonPushRTrigger, 7, 1);

    unsigned turbo_eligible = settings->value(section + "/TurboEligible").toUInt();

    bool checkBoxTurboDPadRState    = turbo_eligible & 0x0001;
    bool checkBoxTurboDPadLState    = turbo_eligible & 0x0002;
    bool checkBoxTurboDPadDState    = turbo_eligible & 0x0004;
    bool checkBoxTurboDPadUState    = turbo_eligible & 0x0008;
    bool checkBoxTurboStartState    = turbo_eligible & 0x0010;
    bool checkBoxTurboZState        = turbo_eligible & 0x0020;
    bool checkBoxTurboBState        = turbo_eligible & 0x0040;
    bool checkBoxTurboAState        = turbo_eligible & 0x0080;
    bool checkBoxTurboCRState       = turbo_eligible & 0x0100;
    bool checkBoxTurboCLState       = turbo_eligible & 0x0200;
    bool checkBoxTurboCDState       = turbo_eligible & 0x0400;
    bool checkBoxTurboCUState       = turbo_eligible & 0x0800;
    bool checkBoxTurboRTriggerState = turbo_eligible & 0x1000;
    bool checkBoxTurboLTriggerState = turbo_eligible & 0x2000;

    QCheckBox *checkBoxTurboA = new QCheckBox(nullptr, this);
    checkBoxTurboA->setChecked(checkBoxTurboAState);
    checkBoxTurboA->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboA, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboA->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboA);
    layout->addWidget(checkBoxTurboA, 2, 2, Qt::AlignCenter);

    QCheckBox *checkBoxTurboB = new QCheckBox(nullptr, this);
    checkBoxTurboB->setChecked(checkBoxTurboBState);
    checkBoxTurboB->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboB, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboB->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboB);
    layout->addWidget(checkBoxTurboB, 3, 2, Qt::AlignCenter);

    QCheckBox *checkBoxTurboZ = new QCheckBox(nullptr, this);
    checkBoxTurboZ->setChecked(checkBoxTurboZState);
    checkBoxTurboZ->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboZ, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboZ->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboZ);
    layout->addWidget(checkBoxTurboZ, 4, 2, Qt::AlignCenter);

    QCheckBox *checkBoxTurboStart = new QCheckBox(nullptr, this);
    checkBoxTurboStart->setChecked(checkBoxTurboStartState);
    checkBoxTurboStart->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboStart, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboStart->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboStart);
    layout->addWidget(checkBoxTurboStart, 5, 2, Qt::AlignCenter);

    QCheckBox *checkBoxTurboLTrigger = new QCheckBox(nullptr, this);
    checkBoxTurboLTrigger->setChecked(checkBoxTurboLTriggerState);
    checkBoxTurboLTrigger->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboLTrigger, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboLTrigger->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboLTrigger);
    layout->addWidget(checkBoxTurboLTrigger, 6, 2, Qt::AlignCenter);

    QCheckBox *checkBoxTurboRTrigger = new QCheckBox(nullptr, this);
    checkBoxTurboRTrigger->setChecked(checkBoxTurboRTriggerState);
    checkBoxTurboRTrigger->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboRTrigger, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboRTrigger->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboRTrigger);
    layout->addWidget(checkBoxTurboRTrigger, 7, 2, Qt::AlignCenter);

    QFrame* lineV = new QFrame(this);
    lineV->setFrameShape(QFrame::VLine);
    lineV->setFrameShadow(QFrame::Sunken);
    layout->addWidget(lineV, 2, 3, 6, 1);

    QLabel *buttonLabelDPadL = new QLabel("DPad Left", this);
    buttonLabelDPadL->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushDPadL = new CustomButton(section, "DPadL", this);
    buttonList.append(buttonPushDPadL);
    layout->addWidget(buttonLabelDPadL, 2, 4);
    layout->addWidget(buttonPushDPadL, 2, 5);

    QLabel *buttonLabelDPadR = new QLabel("DPad Right", this);
    buttonLabelDPadR->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushDPadR = new CustomButton(section, "DPadR", this);
    buttonList.append(buttonPushDPadR);
    layout->addWidget(buttonLabelDPadR, 3, 4);
    layout->addWidget(buttonPushDPadR, 3, 5);

    QLabel *buttonLabelDPadU = new QLabel("DPad Up", this);
    buttonLabelDPadU->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushDPadU = new CustomButton(section, "DPadU", this);
    buttonList.append(buttonPushDPadU);
    layout->addWidget(buttonLabelDPadU, 4, 4);
    layout->addWidget(buttonPushDPadU, 4, 5);

    QLabel *buttonLabelDPadD = new QLabel("DPad Down", this);
    buttonLabelDPadD->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushDPadD = new CustomButton(section, "DPadD", this);
    buttonList.append(buttonPushDPadD);
    layout->addWidget(buttonLabelDPadD, 5, 4);
    layout->addWidget(buttonPushDPadD, 5, 5);

    QCheckBox *checkBoxTurboDPadL = new QCheckBox(nullptr, this);
    checkBoxTurboDPadL->setChecked(checkBoxTurboDPadLState);
    checkBoxTurboDPadL->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboDPadL, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboDPadL->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboDPadL);
    layout->addWidget(checkBoxTurboDPadL, 2, 6, Qt::AlignCenter);

    QCheckBox *checkBoxTurboDPadR = new QCheckBox(nullptr, this);
    checkBoxTurboDPadR->setChecked(checkBoxTurboDPadRState);
    checkBoxTurboDPadR->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboDPadR, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboDPadR->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboDPadR);
    layout->addWidget(checkBoxTurboDPadR, 3, 6, Qt::AlignCenter);

    QCheckBox *checkBoxTurboDPadU = new QCheckBox(nullptr, this);
    checkBoxTurboDPadU->setChecked(checkBoxTurboDPadUState);
    checkBoxTurboDPadU->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboDPadU, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboDPadU->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboDPadU);
    layout->addWidget(checkBoxTurboDPadU, 4, 6, Qt::AlignCenter);

    QCheckBox *checkBoxTurboDPadD = new QCheckBox(nullptr, this);
    checkBoxTurboDPadD->setChecked(checkBoxTurboDPadDState);
    checkBoxTurboDPadD->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboDPadD, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboDPadD->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboDPadD);
    layout->addWidget(checkBoxTurboDPadD, 5, 6, Qt::AlignCenter);

    QFrame* lineV2 = new QFrame(this);
    lineV2->setFrameShape(QFrame::VLine);
    lineV2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(lineV2, 2, 7, 6, 1);

    QLabel *buttonLabelCL = new QLabel("C Left", this);
    buttonLabelCL->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushCL = new CustomButton(section, "CLeft", this);
    buttonList.append(buttonPushCL);
    layout->addWidget(buttonLabelCL, 2, 8);
    layout->addWidget(buttonPushCL, 2, 9);

    QLabel *buttonLabelCR = new QLabel("C Right", this);
    buttonLabelCR->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushCR = new CustomButton(section, "CRight", this);
    buttonList.append(buttonPushCR);
    layout->addWidget(buttonLabelCR, 3, 8);
    layout->addWidget(buttonPushCR, 3, 9);

    QLabel *buttonLabelCU = new QLabel("C Up", this);
    buttonLabelCU->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushCU = new CustomButton(section, "CUp", this);
    buttonList.append(buttonPushCU);
    layout->addWidget(buttonLabelCU, 4, 8);
    layout->addWidget(buttonPushCU, 4, 9);

    QLabel *buttonLabelCD = new QLabel("C Down", this);
    buttonLabelCD->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushCD = new CustomButton(section, "CDown", this);
    buttonList.append(buttonPushCD);
    layout->addWidget(buttonLabelCD, 5, 8);
    layout->addWidget(buttonPushCD, 5, 9);

    QLabel *buttonLabelStickL = new QLabel("Control Stick Left", this);
    buttonLabelStickL->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushStickL = new CustomButton(section, "AxisLeft", this);
    buttonList.append(buttonPushStickL);
    layout->addWidget(buttonLabelStickL, 6, 4);
    layout->addWidget(buttonPushStickL, 6, 5);

    QLabel *buttonLabelStickR = new QLabel("Control Stick Right", this);
    buttonLabelStickR->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushStickR = new CustomButton(section, "AxisRight", this);
    buttonList.append(buttonPushStickR);
    layout->addWidget(buttonLabelStickR, 7, 4);
    layout->addWidget(buttonPushStickR, 7, 5);

    QLabel *buttonLabelStickU = new QLabel("Control Stick Up", this);
    buttonLabelStickU->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushStickU = new CustomButton(section, "AxisUp", this);
    buttonList.append(buttonPushStickU);
    layout->addWidget(buttonLabelStickU, 6, 8);
    layout->addWidget(buttonPushStickU, 6, 9);

    QLabel *buttonLabelStickD = new QLabel("Control Stick Down", this);
    buttonLabelStickD->setAlignment(Qt::AlignCenter);
    CustomButton *buttonPushStickD = new CustomButton(section, "AxisDown", this);
    buttonList.append(buttonPushStickD);
    layout->addWidget(buttonLabelStickD, 7, 8);
    layout->addWidget(buttonPushStickD, 7, 9);

    QCheckBox *checkBoxTurboCL = new QCheckBox(nullptr, this);
    checkBoxTurboCL->setChecked(checkBoxTurboCLState);
    checkBoxTurboCL->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboCL, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboCL->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboCL);
    layout->addWidget(checkBoxTurboCL, 2, 10, Qt::AlignCenter);

    QCheckBox *checkBoxTurboCR = new QCheckBox(nullptr, this);
    checkBoxTurboCR->setChecked(checkBoxTurboCRState);
    checkBoxTurboCR->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboCR, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboCR->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboCR);
    layout->addWidget(checkBoxTurboCR, 3, 10, Qt::AlignCenter);

    QCheckBox *checkBoxTurboCU = new QCheckBox(nullptr, this);
    checkBoxTurboCU->setChecked(checkBoxTurboCUState);
    checkBoxTurboCU->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboCU, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboCU->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboCU);
    layout->addWidget(checkBoxTurboCU, 4, 10, Qt::AlignCenter);

    QCheckBox *checkBoxTurboCD = new QCheckBox(nullptr, this);
    checkBoxTurboCD->setChecked(checkBoxTurboCDState);
    checkBoxTurboCD->setToolTip(turboControllerButtonToolTip);
    connect(checkBoxTurboCD, &QAbstractButton::toggled, [=](bool value) {
        checkBoxTurboCD->setChecked(value);
    });
    checkBoxList.append(checkBoxTurboCD);
    layout->addWidget(checkBoxTurboCD, 5, 10, Qt::AlignCenter);

    QFrame* lineH2 = new QFrame(this);
    lineH2->setFrameShape(QFrame::HLine);
    lineH2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(lineH2, 8, 0, 1, 11);

    QLabel *buttonLabelAbsoluteXYAxis = new QLabel("Absolute X/Y Axis (Keyboard Only)", this);
    buttonLabelAbsoluteXYAxis->setWordWrap(true);
    buttonLabelAbsoluteXYAxis->setAlignment(Qt::AlignCenter);
    CustomButton *buttonAbsoluteXYAxis = new CustomButton(section, "AbsoluteXYAxis", this);
    buttonList.append(buttonAbsoluteXYAxis);
    layout->addWidget(buttonLabelAbsoluteXYAxis, 9, 0);
    layout->addWidget(buttonAbsoluteXYAxis, 9, 1, 1, 2);

    QCheckBox *checkBoxToggleAbsoluteXYAxis = new QCheckBox("Toggle", this);
    bool checkBoxToggleAbsoluteXYAxisState = settings->value(section + "/ToggleAbsoluteXYAxis").toBool();
    checkBoxToggleAbsoluteXYAxis->setToolTip(toggleToolTip);
    checkBoxToggleAbsoluteXYAxis->setChecked(checkBoxToggleAbsoluteXYAxisState);
    connect(checkBoxToggleAbsoluteXYAxis, &QAbstractButton::toggled, [=](bool value) {
        checkBoxToggleAbsoluteXYAxis->setChecked(value);
    });
    checkBoxList.append(checkBoxToggleAbsoluteXYAxis);
    layout->addWidget(checkBoxToggleAbsoluteXYAxis, 9, 3, 1, 2, Qt::AlignCenter);

    QFrame* lineV3 = new QFrame(this);
    lineV3->setFrameShape(QFrame::VLine);
    lineV3->setFrameShadow(QFrame::Sunken);
    layout->addWidget(lineV3, 9, 5, 2, 1);

    QLabel *buttonLabelTurbo = new QLabel("Turbo", this);
    buttonLabelTurbo->setAlignment(Qt::AlignCenter);
    CustomButton *buttonTurbo = new CustomButton(section, "Turbo", this);
    buttonList.append(buttonTurbo);
    layout->addWidget(buttonLabelTurbo, 9, 5, 1, 3);
    layout->addWidget(buttonTurbo, 9, 7, 1, 2);

    QCheckBox *checkBoxToggleTurbo = new QCheckBox("Toggle", this);
    bool checkBoxToggleTurboState = settings->value(section + "/ToggleTurbo").toBool();
    checkBoxToggleTurbo->setToolTip(toggleToolTip);
    checkBoxToggleTurbo->setChecked(checkBoxToggleTurboState);
    connect(checkBoxToggleTurbo, &QAbstractButton::toggled, [=](bool value) {
        checkBoxToggleTurbo->setChecked(value);
    });
    checkBoxList.append(checkBoxToggleTurbo);
    layout->addWidget(checkBoxToggleTurbo, 9, 9, 1, 2, Qt::AlignCenter);

    QLabel *buttonLabelAbsoluteXYAxisSensitivity = new QLabel("Sensitivity", this);
    buttonLabelAbsoluteXYAxisSensitivity->setAlignment(Qt::AlignCenter);
    QLabel *buttonLabelAbsoluteXYAxisSensitivityValue = new QLabel(this);
    int AbsoluteXYAxisSensitivityValueValue = settings->value(section + "/AbsoluteXYAxisSensitivity").toInt();
    QString buttonLabelAbsoluteXYAxisSensitivityTense = (AbsoluteXYAxisSensitivityValueValue != 1) ? " Units/Frame" : " Unit/Frame";
    buttonLabelAbsoluteXYAxisSensitivityValue->setText(QString::number(AbsoluteXYAxisSensitivityValueValue) + buttonLabelAbsoluteXYAxisSensitivityTense);
    buttonLabelAbsoluteXYAxisSensitivityValue->setAlignment(Qt::AlignCenter);
    QSlider *sliderAbsoluteXYAxisSensitivity = new QSlider(Qt::Horizontal, this);
    sliderAbsoluteXYAxisSensitivity->setMinimum(1);
    sliderAbsoluteXYAxisSensitivity->setMaximum(10);
    sliderAbsoluteXYAxisSensitivity->setTickPosition(QSlider::TicksBothSides);
    sliderAbsoluteXYAxisSensitivity->setTickInterval(1);
    sliderAbsoluteXYAxisSensitivity->setSliderPosition(AbsoluteXYAxisSensitivityValueValue);
    sliderAbsoluteXYAxisSensitivity->setToolTip(absoluteXYAxisSensitivitySliderToolTip);
    connect(sliderAbsoluteXYAxisSensitivity, &QSlider::valueChanged, [=](int value) {
        QString buttonLabelAbsoluteXYAxisSensitivityTense = (value != 1) ? " Units/Frame" : " Unit/Frame";
        buttonLabelAbsoluteXYAxisSensitivityValue->setText(QString::number(value) + buttonLabelAbsoluteXYAxisSensitivityTense);
    });
    sliderList.append(sliderAbsoluteXYAxisSensitivity);

    layout->addWidget(buttonLabelAbsoluteXYAxisSensitivity, 10, 0);
    layout->addWidget(buttonLabelAbsoluteXYAxisSensitivityValue, 10, 1, 1, 2);
    layout->addWidget(sliderAbsoluteXYAxisSensitivity, 10, 3, 1, 2);

    QLabel *buttonLabelTurboRate = new QLabel("Frequency", this);
    buttonLabelTurboRate->setAlignment(Qt::AlignCenter);
    QLabel *buttonLabelTurboRateValue = new QLabel(this);
    unsigned turboRateValue = settings->value(section + "/TurboRate").toUInt() / 2;
    buttonLabelTurboRateValue->setText("Wait " + QString::number(turboRateValue * 2) + " Frames");
    buttonLabelTurboRateValue->setAlignment(Qt::AlignCenter);
    QSlider *sliderTurboRate = new QSlider(Qt::Horizontal, this);
    sliderTurboRate->setInvertedAppearance(true);
    sliderTurboRate->setInvertedControls(true);
    sliderTurboRate->setMinimum(1);
    sliderTurboRate->setMaximum(8);
    sliderTurboRate->setTickPosition(QSlider::TicksBothSides);
    sliderTurboRate->setTickInterval(1);
    sliderTurboRate->setSliderPosition(turboRateValue);
    sliderTurboRate->setToolTip(turboRateSliderToolTip);
    connect(sliderTurboRate, &QSlider::valueChanged, [=](unsigned value) {
        buttonLabelTurboRateValue->setText("Wait " + QString::number(value * 2) + " Frames");
    });
    sliderList.append(sliderTurboRate);

    layout->addWidget(buttonLabelTurboRate, 10, 5, 1, 3);
    layout->addWidget(buttonLabelTurboRateValue, 10, 7, 1, 2);
    layout->addWidget(sliderTurboRate, 10, 9, 1, 2);

    QFrame* lineH3 = new QFrame(this);
    lineH3->setFrameShape(QFrame::HLine);
    lineH3->setFrameShadow(QFrame::Sunken);
    layout->addWidget(lineH3, 11, 0, 1, 11);

    QLabel *buttonLabelDeadzone = new QLabel("Deadzone", this);
    buttonLabelDeadzone->setAlignment(Qt::AlignCenter);
    QLabel *buttonLabelDeadzoneValue = new QLabel(this);
    float deadzoneValue = settings->value(section + "/Deadzone").toFloat();
    buttonLabelDeadzoneValue->setText(QString::number(deadzoneValue) + "%");
    buttonLabelDeadzoneValue->setAlignment(Qt::AlignCenter);
    QSlider *sliderDeadzone = new QSlider(Qt::Horizontal, this);
    sliderDeadzone->setMinimum(0);
    sliderDeadzone->setMaximum(250);
    sliderDeadzone->setTickPosition(QSlider::TicksBothSides);
    sliderDeadzone->setTickInterval(5);
    sliderDeadzone->setSliderPosition((int)(deadzoneValue * 10.0));
    connect(sliderDeadzone, &QSlider::valueChanged, [=](int value) {
        float percent = value / 10.0;
        buttonLabelDeadzoneValue->setText(QString::number(percent, 'f', 1) + "%");
    });
    sliderList.append(sliderDeadzone);

    layout->addWidget(buttonLabelDeadzone, 12, 0);
    layout->addWidget(buttonLabelDeadzoneValue, 12, 1, 1, 2);
    layout->addWidget(sliderDeadzone, 12, 3, 1, 8);

    QLabel *buttonLabelSensitivity = new QLabel("Analog Sensitivity", this);
    buttonLabelSensitivity->setAlignment(Qt::AlignCenter);
    QLabel *buttonLabelSensitivityValue = new QLabel(this);
    float sensitivityValue = settings->value(section + "/Sensitivity").toFloat();
    buttonLabelSensitivityValue->setText(QString::number(sensitivityValue) + "%");
    buttonLabelSensitivityValue->setAlignment(Qt::AlignCenter);
    QSlider *sliderSensitivity = new QSlider(Qt::Horizontal, this);
    sliderSensitivity->setMinimum(800);
    sliderSensitivity->setMaximum(1200);
    sliderSensitivity->setTickPosition(QSlider::TicksBothSides);
    sliderSensitivity->setTickInterval(5);
    sliderSensitivity->setSliderPosition((int)(sensitivityValue * 10.0));
    connect(sliderSensitivity, &QSlider::valueChanged, [=](int value) {
        float percent = value / 10.0;
        buttonLabelSensitivityValue->setText(QString::number(percent, 'f', 1) + "%");
    });
    sliderList.append(sliderSensitivity);

    layout->addWidget(buttonLabelSensitivity, 13, 0);
    layout->addWidget(buttonLabelSensitivityValue, 13, 1, 1, 2);
    layout->addWidget(sliderSensitivity, 13, 3, 1, 8);

    QFrame* lineH4 = new QFrame(this);
    lineH4->setFrameShape(QFrame::HLine);
    lineH4->setFrameShadow(QFrame::Sunken);
    layout->addWidget(lineH4, 14, 0, 1, 11);

    QPushButton *buttonPushSave = new QPushButton("Save and Close", this);
    connect(buttonPushSave, &QPushButton::released, [=]() {
        const QString saveSection = profileName->text();
        if (!saveSection.startsWith("Auto-") && !saveSection.isEmpty() && (!settings->contains(saveSection + "/A") || !profileName->isEnabled())) {
            QList<int> value;
            for (int i = 0; i < buttonList.size(); ++i) {
                value.clear();
                switch (buttonList.at(i)->type) {
                    case 0:
                        value.insert(0, buttonList.at(i)->key);
                        value.insert(1, 0);
                        break;
                    case 1:
                        value.insert(0, buttonList.at(i)->button);
                        value.insert(1, 1);
                        break;
                    case 2:
                        value.insert(0, buttonList.at(i)->axis);
                        value.insert(1, 2);
                        value.insert(2, buttonList.at(i)->axisValue);
                        break;
                    case 3:
                        value.insert(0, buttonList.at(i)->joystick_hat);
                        value.insert(1, 3);
                        value.insert(2, buttonList.at(i)->axisValue);
                        break;
                    case 4:
                        value.insert(0, buttonList.at(i)->joystick_button);
                        value.insert(1, 4);
                        break;
                    case 5:
                        value.insert(0, buttonList.at(i)->joystick_axis);
                        value.insert(1, 5);
                        value.insert(2, buttonList.at(i)->axisValue);
                        break;
                }
                settings->setValue(saveSection + "/" + buttonList.at(i)->item, QVariant::fromValue(value));
            }
            float percent = sliderDeadzone->value() / 10.0;
            settings->setValue(saveSection + "/Deadzone", percent);
            percent = sliderSensitivity->value() / 10.0;
            settings->setValue(saveSection + "/Sensitivity", percent);
            settings->setValue(saveSection + "/ToggleTurbo", checkBoxToggleTurbo->isChecked());
            settings->setValue(saveSection + "/ToggleAbsoluteXYAxis", checkBoxToggleAbsoluteXYAxis->isChecked());
            settings->setValue(saveSection + "/AbsoluteXYAxisSensitivity", sliderAbsoluteXYAxisSensitivity->value());
            settings->setValue(saveSection + "/TurboRate", sliderTurboRate->value() * 2);
            settings->setValue(saveSection + "/TurboEligible",
                checkBoxTurboDPadR->isChecked()    << 0  | checkBoxTurboDPadL->isChecked()    << 1  |
                checkBoxTurboDPadD->isChecked()    << 2  | checkBoxTurboDPadU->isChecked()    << 3  |
                checkBoxTurboStart->isChecked()    << 4  | checkBoxTurboZ->isChecked()        << 5  |
                checkBoxTurboB->isChecked()        << 6  | checkBoxTurboA->isChecked()        << 7  |
                checkBoxTurboCR->isChecked()       << 8  | checkBoxTurboCL->isChecked()       << 9  |
                checkBoxTurboCD->isChecked()       << 10 | checkBoxTurboCU->isChecked()       << 11 |
                checkBoxTurboRTrigger->isChecked() << 12 | checkBoxTurboLTrigger->isChecked() << 13
            );
            this->done(1);
        }
        else {
            QMessageBox msgBox;
            msgBox.setText("Invalid profile name.");
            msgBox.exec();
        }
    });
    pushButtonList.append(buttonPushSave);
    layout->addWidget(buttonPushSave, 15, 0, 1, 3);
    QPushButton *buttonPushClose = new QPushButton("Close Without Saving", this);
    connect(buttonPushClose, &QPushButton::released, [=]() {
        this->done(1);
    });
    pushButtonList.append(buttonPushClose);
    layout->addWidget(buttonPushClose, 15, 8, 1, 3);

    setLayout(layout);
    setWindowTitle(tr("Profile Editor"));
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

ConfigDialog::ConfigDialog()
{
    unsigned int i;

    tabWidget = new QTabWidget(this);
    tabWidget->setUsesScrollButtons(0);
    for (i = 1; i < 5; ++i) {
        controllerTabs[i-1] = new ControllerTab(i);
        tabWidget->addTab(controllerTabs[i-1], "Controller " + QString::number(i));
    }

    tabWidget->addTab(new ProfileTab(controllerTabs), tr("Manage Profiles"));
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
    setWindowTitle(tr("Controller Configuration"));
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
}
