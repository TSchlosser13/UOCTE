#include "exportJpeg.hpp"

using namespace std;

bool exportSlicesAsJpeg(const oct_scan* scan, string filename_base, std::vector<int> contourList, QColor contourColor)
{
    // shorthand
    size_t size = scan->tomogram.width() * scan->tomogram.height() * scan->tomogram.depth();

    // find min and max intensity value, discard upper and lower 1% as outliers
    unique_ptr<uint8_t []> d(new uint8_t[size]);
    unique_ptr<uint8_t []> d_asRgba(new uint8_t[size*4]);
    copy_n(scan->tomogram.data(), size, d.get());
    uint8_t *p01 = d.get() + size / 100;
    uint8_t *p99 = d.get() + size - (size + 99) / 100;
    nth_element(d.get(), p01, d.get() + size);
    nth_element(d.get(), p99, d.get() + size);
    uint8_t minv = *p01, maxv = *p99;

    for (size_t k = 0; k != size; ++k) {
        // turn image negative and maximize contrast
        d[k] = uint8_t(numeric_limits<uint8_t>::max() * min(1.0, max(0.0, (1.0 - double(scan->tomogram.data()[k] - minv) / (maxv - minv)))));

        //convert grayscale to RGBA
        d_asRgba[4*k+0] = d[k];
        d_asRgba[4*k+1] = d[k];
        d_asRgba[4*k+2] = d[k];
        d_asRgba[4*k+3] = 255; //no transparency
    }

    //export every layer of the 3d volume as 2d image
    for (size_t z = 0; z != scan->tomogram.depth(); ++z)
    {
        //draw contours to image
        for (int contourID : contourList)
        {
            auto it = scan->contours.begin();
            advance(it, contourID);
            const image<float> &c = it->second;

            //ignore NaN as last contour (found in E2E files)
            //use center of image as example for validation
            float test = c(c.width()/2,c.height()/2);
            if (test != test)
                continue;

            for (size_t x = 0; x != scan->tomogram.width(); ++x)
            {
                float contourValue = c(x,z);
                if (contourValue != contourValue)
                    continue;
                int y = (int)(contourValue + 0.5f); //round value to full pixels

                int pos = ((z*scan->tomogram.height() + y )* scan->tomogram.width() + x)*4;
                int posAbove = pos - 4*scan->tomogram.width();
                int posBelow = pos + 4*scan->tomogram.width();

                d_asRgba[pos+0] = contourColor.red();
                d_asRgba[pos+1] = contourColor.green();
                d_asRgba[pos+2] = contourColor.blue();
                d_asRgba[pos+3] = contourColor.alpha();

                //make line thickness 3 pixel
                d_asRgba[posAbove+0] = contourColor.red();
                d_asRgba[posAbove+1] = contourColor.green();
                d_asRgba[posAbove+2] = contourColor.blue();
                d_asRgba[posAbove+3] = contourColor.alpha();
                d_asRgba[posBelow+0] = contourColor.red();
                d_asRgba[posBelow+1] = contourColor.green();
                d_asRgba[posBelow+2] = contourColor.blue();
                d_asRgba[posBelow+3] = contourColor.alpha();
            }
        }

        //export to file
        stringstream filename;
        filename << filename_base << "slice" << z << ".jpg";
        jpge::compress_image_to_jpeg_file(filename.str().c_str(), scan->tomogram.width(), scan->tomogram.height(), 4, &d_asRgba[z*scan->tomogram.height()*scan->tomogram.width()*4]);
    }
}
