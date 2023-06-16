#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>

 
#define RGB 0
#define HSV 1
//#include "ImageInfo.h"

using namespace cv;

//struct storing edge points of object detected on picture
struct MyNode {
     int left, high, right, low;
     std::unique_ptr<MyNode> next;
     MyNode(int a, int b, int c = 0, int d = 0) : left{a},high{b}, right{c},low{d}, next{nullptr} {}
 };


//list of MyNodes
struct List {
     List() : head{nullptr} {};
 
     void push(int a, int b, int c=0, int d=0) {
        auto temp{std::make_unique<MyNode>(a, b, c, d)};
        if(head) {
            temp->next = std::move(head);
            head = std::move(temp);
 
        } else {
            head = std::move(temp);
        }
     }

     MyNode* getHead(){
        return head.get();
     }

     //to avoid stack overflow due to recursive destroing nodes
     void clean() {
         while(head) 
             head = std::move(head->next);
        }
     ~List() {clean();}
 
 private:
     std::unique_ptr<MyNode> head;
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
    List detectedObjects;
    Mat image;
    int colorRepresentation;
public:
    ImageInfo (std::string imagePath, int representanion);
    ~ImageInfo();
    int getImageRows() {return this->image.rows;}
    void displayImage();
    MyNode process(int row, int col);
    int findObjects();
};

ImageInfo::ImageInfo(std::string imagePath, int representanion){
    this->image = imread(imagePath);
    if ( !image.data )
    {
        throw std::invalid_argument("Incorrect file path!");
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
    for (int i = 0; i < this->image.rows; i++) // To delete the inner
                                // arrays
        delete[] this->processed[i];
    delete[] this->processed; 
}

void ImageInfo::displayImage(){
    namedWindow("Display Image", 0 );
    imshow("Display Image", this->image);
    waitKey(0);
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
    this->detectedObjects.clean();
    int objects = 0;
    MyNode temp(0,0,0,0);
    for (int i = 0; i < this->image.rows; i++)
        for (int j = 0; j< this->image.cols; j++){
            if (this->processed[i][j] == false){
                temp = process(i,j);

                if (temp.right - temp.left < this->image.cols/3 && abs(temp.high - temp.low) < this->image.rows/3
                    && temp.right - temp.left > this->image.cols/100 && abs(temp.high - temp.low) > this->image.rows/100 ){
                     objects++;
                     rectangle(this->image,Point(temp.left,temp.high),Point(temp.right,temp.low),Scalar(0,0,0),3,LINE_4);
                }
                   
            }
        }
    return objects;
}

//-----------------------------------------------------------------------

int main(int argc, char** argv )
{
    try{
        std::cout<<"Gimmie path to the photo here -> ";
        std::string photoPath;
        std::cin>>photoPath;
        auto picture = new ImageInfo(photoPath,0);

        std::cout<<picture->findObjects();

        picture->displayImage();
    }
    catch (std::invalid_argument& e){    
        std::cerr << e.what() << std::endl;
        return -1;
    }
    //C:\Users\Ja\Pictures\detect_paint_holds.png
    //C:\Users\Ja\Pictures\cartoon_wall.png
    return 0;
}