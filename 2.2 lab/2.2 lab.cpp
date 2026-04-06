#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <Windows.h>
#include <fstream>
#include <conio.h>
#include <filesystem>
using namespace std;
namespace fs = filesystem;

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void SetCursorBlinking(bool isBlinking)
{
    CONSOLE_CURSOR_INFO structCursorInfo;
    GetConsoleCursorInfo(hConsole, &structCursorInfo);
    structCursorInfo.bVisible = isBlinking;
    SetConsoleCursorInfo(hConsole, &structCursorInfo);
}

void GetCommand(string& command)
{
    cout << "Encode -> 1\n";
    cout << "Decode -> 2\n";
    cout << "Enter command: ";
    char ch;
    while (true)
    {
        ch = _getch();
        if (ch == '1')
        {
            command = "-e";
            cout << ch << '\n';
            break;
        }
        else if (ch == '2')
        {
            command = "-d";
            cout << ch << '\n';
            break;
        }
    }
}

void GetFilePath(ifstream& file, string& filePath)
{
    while (true)
    {
        cout << "Enter file path: ";
        getline(cin, filePath);
        file.open(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            cerr << "Couldn't open the file!" << endl;
        else
            break;
    }
}

#pragma pack(push, 1)
struct WAVHeader
{
    char chunkId[4] = { 'R', 'I', 'F', 'F' };
    uint32_t chunkSize = 1;
    char format[4] = { 'W', 'A', 'V', 'E' };
    char subchunk1Id[4] = { 'f', 'm', 't', ' ' };
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 2;
    uint32_t sampleRate = 44100;
    uint32_t byteRate = 44100 * 2 * 2;
    uint16_t blockAlign = 2 * 2;
    uint16_t bitsPerSample = 16;
    char subchunk2Id[4] = { 'd', 'a', 't', 'a' };
    uint32_t subchunk2Size = 1;
};
#pragma pack(pop)

void SetWAVHeader(WAVHeader& header, uint32_t size)
{
    header.subchunk1Size = 16;
    header.audioFormat = 1;
    header.numChannels = 2;
    header.sampleRate = 44100;
    header.bitsPerSample = 16;
    header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
    header.byteRate = header.sampleRate * header.blockAlign;
    header.subchunk2Size = size;
    header.chunkSize = 36 + header.subchunk2Size;
}

void Encode(uint32_t size, vector<char> buffer, string filePath)
{
    for (int i = 0; i <= 9; ++i)
    {
        cout << "\rProgress: " << i * 10 << "%";
        Sleep(200);
    }

    WAVHeader header;
    SetWAVHeader(header, size);

    fs::path fP = filePath;
    vector<char> fPextension; //Расширение файла
    for (char ch : fP.extension().string())
        fPextension.push_back(ch);

    string newfP = fP.parent_path().string() + "\\" + fP.stem().string() + ".wav";
    ofstream outFile(newfP, std::ios::binary);
    if (outFile.is_open())
    {
        outFile.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));
        if (fPextension.size() < 10)
        {
            while (fPextension.size() < 10)
                fPextension.push_back(' ');
        }
        outFile.write(fPextension.data(), fPextension.size());
        outFile.write(buffer.data(), buffer.size());
        outFile.close();
        fs::remove(filePath);
        cout << "\rProgress: " << 100 << "%" << endl;
        cout << "Successful encoded!" << endl;
    }
    else
    {
        cout << endl;
        cerr << "Error encoding!" << endl;
    }
}

void Decode(uint32_t size, vector<char> buffer, string filePath)
{
    for (int i = 0; i <= 9; ++i)
    {
        cout << "\rProgress: " << i * 10 << "%";
        Sleep(200);
    }

    fs::path fP = filePath;
    string newfP = fP.parent_path().string() + "\\" + fP.stem().string();
    for (int i = 0; i < 10; i++)
    {
        if (buffer[i] != ' ')
            newfP += buffer[i];
    }
    ofstream outFile(newfP, std::ios::binary);
    if (outFile.is_open())
    {
        outFile.write(&buffer[10], buffer.size() - 10);
        outFile.close();
        fs::remove(filePath);
        cout << "\rProgress: " << 100 << "%" << endl;
        cout << "Successful decoded!" << endl;
    }
    else
    {
        cout << endl;
        cerr << "Error decoding!" << endl;
    }
}

void WaitStart(string command)
{
    string message = "Press 1 to start ";
    if (command == "-e" || command == "-encode")
        message += "encoding";
    else
        message += "decoding";
    cout << message << endl;

    while (true)
    {
        char ch = _getch();
        if (ch == '1')
        {
            break;
        }
    }
}

void ReadBytes(string command, ifstream& file, string filePath)
{
    if (command == "-e" || command == "-encode")
    {
        //Определяем размер файла по текущей позиции указателя
        uint32_t size = file.tellg();
        vector<char> buffer(size);
        //Перемещаем указатель обратно в начало
        file.seekg(0, std::ios::beg);

        if (file.read(buffer.data(), size))
            cout << "Successful readed " << size << " bytes!" << endl;
        else
            cerr << "Error reading bytes!";
        file.close();

        WaitStart(command);
        Encode(size, buffer, filePath);
    }
    else
    {
        uint32_t size = (uint32_t)file.tellg() - sizeof(WAVHeader);
        vector<char> buffer(size);
        file.seekg(sizeof(WAVHeader), ios::beg);

        if (file.read(buffer.data(), size))
            cout << "Successful readed " << size << " bytes!" << endl;
        else
            cerr << "Error reading bytes!";
        file.close();

        WaitStart(command);
        Decode(size, buffer, filePath);
    }
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "Russian");
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251); //Для возможности ввода русских символов
    system("cls");

    string command;
    if (argc < 2)
        GetCommand(command);
    else
        command = argv[1];

    string filePath = "Password.txt";
    ifstream file;
    if (argc < 3)
    {
        GetFilePath(file, filePath);
    }
    else
    {
        filePath = argv[2];
        file.open(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            cerr << "Couldn't open the file!" << endl;
            GetFilePath(file, filePath);
        }
    }

    SetCursorBlinking(false);
    
    ReadBytes(command, file, filePath);

    return 0;
}
