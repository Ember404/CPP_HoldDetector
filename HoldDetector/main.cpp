#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <fstream> 
#include <vector>

#define RGB 0
#define HSV 1

using namespace cv;

//struct storing edge points of object detected on picture
struct MyNode {
     int left, high, right, low;
     std::unique_ptr<MyNode> next;
     MyNode(int a, int b, int c = 0, int d = 0) : left{a},high{b}, right{c},low{d}, next{nullptr} {}
 };


//key function, deciding wether there is contrast between pixels
bool isSimilar(Vec3b pixel, Vec3b checkedPixel, int colorRepresentation){
    if (colorRepresentation)
    //checking if hue is ismilar (assuming value 0 is similar to 180)
        if (abs(pixel[0]-checkedPixel[0])<9||abs(pixel[0]-checkedPixel[0])>171)
            //checking if shade is somewhat similar
            if(abs(pixel[1]-checkedPixel[1])<100 && abs(pixel[2]-checkedPixel[2])<100)
                return true;
        else 
            return false;


    return false;
}

//struct to store points in set
struct Coordinates {
    int row,col;
    Coordinates(int a=0, int b=0) : row{a},col{b} {}
    const bool operator<(const Coordinates& rhs)const;
 };

const bool Coordinates::operator<(const Coordinates& rhs) const 
{
    if (this->row != rhs.row)
        return this->row < rhs.row;
    return this->col < rhs.col; 
}


//---------------------IMAGE INFO-------------------------------
//class storing all informations about picture
class ImageInfo{
private:
    bool** processed;
    std::vector <MyNode> detectedObjects;
    Mat image;
    int colorRepresentation;
public:
    ImageInfo (std::string imagePath, int representanion);
    ~ImageInfo();
    int getImageRows() {return this->image.rows;}
    void displayImage();
    MyNode process(int row, int col);
    int findObjects();
    void saveObjects(std::string outputFileName);
};

ImageInfo::ImageInfo(std::string imagePath, int representanion){
    this->image = imread(imagePath);
    if ( !image.data )
    {
        throw std::invalid_argument("Incorrect file path!");
    }

    this->colorRepresentation = representanion;

    if (representanion){
        cvtColor(this->image, this->image, COLOR_BGR2HSV);
    }

    this->processed = new bool*[this->image.rows];
    for (int i = 0; i < this->image.rows; i++) {
 
        // Declare a memory block
        // of size n
        this->processed[i] = new bool[this->image.cols];

        for (int j = 0; j < this->image.cols; j++)
            this->processed[i][j] = false;
    }
}

ImageInfo::~ImageInfo(){
    for (int i = 0; i < this->image.rows; i++)
        delete[] this->processed[i];
    delete[] this->processed; 
}

void ImageInfo::displayImage(){
    namedWindow("Display Image", 0 );
    Mat bgr_image;
    cvtColor(this->image,bgr_image,COLOR_HSV2BGR);
    imshow("Display Image", bgr_image);
    waitKey(0);
}

void ImageInfo::saveObjects(std::string outputFileName){
    std::fstream outputFile;
    outputFile.open(outputFileName.c_str(),std::ios::out);

    if (!outputFile){
        std::cout << "Unable to create output file!\n";
        return;
    }

    int x,y;

    for (int i = 0; i < this->detectedObjects.size(); i++){
        x = (this->detectedObjects[i].left+this->detectedObjects[i].right)/2;
        y = (this->detectedObjects[i].low +this->detectedObjects[i].high)/2;
        outputFile << x << "\t" << y <<"\n";
        //std::cout << x << "\t" << y <<"\n";
    }

    outputFile.close();
}

MyNode ImageInfo::process(int row, int col){
    std::set<Coordinates> toCheck;
    toCheck.insert(Coordinates(row,col));
    MyNode shape_size(this->image.cols,0,0,this->image.rows);

    
    while (!toCheck.empty()){
        auto pom = toCheck.begin();

        shape_size.right = (pom->col > shape_size.right) ? pom->col : shape_size.right;
        shape_size.left = (pom->col < shape_size.left) ? pom->col : shape_size.left;
        shape_size.high = (pom->row > shape_size.high) ? pom->row : shape_size.high;
        shape_size.low = (pom->row < shape_size.low) ? pom->row : shape_size.low;



        if (pom->col > 0)
                if (processed[pom->row][pom->col -1] != true)
                    if (isSimilar(this->image.at<Vec3b>(pom->row,pom->col - 1), this->image.at<Vec3b>(row,col),this->colorRepresentation)){
                        this->processed[pom->row][pom->col -1] = true;
                        toCheck.insert(Coordinates(pom->row,pom->col - 1));
                    }

        if (pom->row > 0)
            if (processed[pom->row - 1][pom->col] != true)
                if (isSimilar(this->image.at<Vec3b>(pom->row - 1,pom->col), this->image.at<Vec3b>(row,col),this->colorRepresentation)){
                        this->processed[pom->row - 1][pom->col] = true;
                        toCheck.insert(Coordinates(pom->row - 1,pom->col));
                }

        if (pom->row < this->image.rows-1)
            if (processed[pom->row + 1][pom->col] != true)
                if (isSimilar(this->image.at<Vec3b>(pom->row + 1,pom->col), this->image.at<Vec3b>(row,col),this->colorRepresentation)){
                        this->processed[pom->row + 1][pom->col] = true;
                        toCheck.insert(Coordinates(pom->row + 1,pom->col));
                }

        if (pom->col < this->image.cols - 1)
            if (processed[pom->row][pom->col + 1] != true)
                if (isSimilar(this->image.at<Vec3b>(pom->row,pom->col + 1), this->image.at<Vec3b>(row,col),this->colorRepresentation)){
                        this->processed[pom->row][pom->col] = true;
                        toCheck.insert(Coordinates(pom->row,pom->col + 1));
                }
        toCheck.erase(pom);
    }
    return shape_size;
}

int ImageInfo::findObjects(){
    this->detectedObjects.clear();
    int objects = 0;
    MyNode temp(0,0,0,0);

    for (int i = 0; i < this->image.rows; i++)
        for (int j = 0; j< this->image.cols; j++){
            if (this->processed[i][j] == false){
                temp = process(i,j);

                if (temp.right - temp.left < this->image.cols/3 
                    && abs(temp.high - temp.low) < this->image.rows/3
                    && temp.right - temp.left > this->image.cols/100 
                    && abs(temp.high - temp.low) > this->image.rows/100 )
                {
                    objects++;
                    detectedObjects.push_back(MyNode(temp.left,
                    temp.high, temp.right, temp.low));
                    rectangle(this->image,
                        Point(temp.left,temp.high),
                        Point(temp.right,temp.low),
                        Scalar(0,0,0),3,LINE_4
                    );
                }
                   
            }
        }
    return objects;
}

//-----------------------------------------------------------------------

int main(int argc, char** argv )
{
    try{
        std::string photoPath,outputName;

        if (argc == 1){
            std::cout<<"Input path to the photo here -> ";
            std::cin >> photoPath;
            std::cout<<"Input path to the output file here -> ";
            std::cin >> outputName;
        } else if (argc == 3){
            photoPath = argv[1];
            outputName = argv[2];
        } else {
            std::cout<<"Error: Wrong number of arguments!";
            return -1;
        }

        auto picture = new ImageInfo(photoPath,1);

        std::cout << picture->findObjects();

        //picture->displayImage();

        picture->saveObjects(outputName);
    }
    catch (std::invalid_argument& e){    
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}