#ifndef YARRGUI_SPEC_CPP
#define YARRGUI_SPEC_CPP

void YarrGui::on_device_comboBox_currentIndexChanged(int index) {
    if (specVec.size()) {
        ui->specid_value->setNum(specVec[index]->getId());
        ui->bar0_value->setNum(specVec[index]->getBarSize(0));
        ui->bar4_value->setNum(specVec[index]->getBarSize(4));
    }
}

void YarrGui::on_init_button_clicked() {
    int index = ui->device_comboBox->currentIndex();
    if (specVec.size() == 0 || index > specVec.size()) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Device not found!");
    } else {
        specVec[index]->init(index);
        if (specVec[index]->isInitialized()) {
            ui->specid_value->setNum(specVec[index]->getId());
            ui->bar0_value->setNum(specVec[index]->getBarSize(0));
            ui->bar4_value->setNum(specVec[index]->getBarSize(4));
            ui->main_tabWidget->setTabEnabled(1, true);
            ui->main_tabWidget->setTabEnabled(2, true);
            ui->addFuncButton->setEnabled(true);
            tx = new TxCore(specVec[index]);
            rx = new RxCore(specVec[index]);
            bk = new Bookkeeper(tx, rx);
        } else {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Initialization not successful!");
        }
    }
}

void YarrGui::on_progfile_button_clicked() {
    QString filename = QFileDialog::getOpenFileName(this, tr("Select bit-file"), "./", tr("Bit File(*.bit)"));
    std::fstream file(filename.toStdString().c_str(), std::fstream::in);
    if (!file && BitFile::checkFile(file)) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Selected bit file looks bad!");
        ui->progfile_name->setText("");
        return;
    }
    ui->progfile_name->setText(filename);
}

void YarrGui::on_prog_button_clicked() {
    int index = ui->device_comboBox->currentIndex();
    if (specVec.size() == 0 || index > specVec.size()) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Device not found!");
    } else {
        if (!specVec[index]->isInitialized()) {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Spec not initiliazed!");
            return;
        }

//        std::fstream file(ui->progfile_name->text().toStdString().c_str(), std::fstream::in);
        std::fstream file;
        if(ui->revABitfRadio->isChecked()){
            file.open("../hdl/syn/yarr_quad_fei4_revA.bit", std::fstream::in);
        }else if(ui->revBBitfRadio->isChecked()){
            file.open("../hdl/syn/yarr_quad_fei4_revB.bit", std::fstream::in);
        }else{
            std::cerr << "ERROR - must choose one bitfile (rev A or rev B) - aborting... " << std::endl;
            return;
        }
        if (!file) {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Problem opening File!");
            return;
        }

        size_t size = BitFile::getSize(file);

        // Read file into buffer
        char *buffer = new char[size];
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);

        // Program FPGA
        int wrote = specVec[index]->progFpga(buffer, size);
        if (wrote != size) {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "FPGA not succesfully programmed!");
        }
        delete buffer;
        file.close();
    }
}

#endif
