#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <cmath>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <list>
#include <vector>
#include <iterator>
#include <utility>
#include <cstdlib>

using std::list;
using std::vector;

const float PI = 3.1415926;
const float G = 0.3;

float time_coefficient = 1;

int WIDTH = 1300;
int HEIGHT = 900;


class vec2
{
public:
    float x, y;

    vec2(float x_coord = 0, float y_coord = 0): x{x_coord}, y{y_coord}
    {
    }

    void operator+=(vec2 &vector)
    {
        x += vector.x;
        y += vector.y;
    }

    void operator+=(vec2 &&vector)
    {
        x += vector.x;
        y += vector.y;
    }

    friend vec2 operator+(vec2 vector1, vec2 vector2)
    {
        return vec2(vector1.x + vector2.x, vector1.y + vector2.y);
    }

    friend vec2 operator*(vec2 vector, float k)
    {
        return vec2(vector.x * k, vector.y * k);
    }

    bool operator==(vec2 &vector)
    {
        return (x == vector.x && y == vector.y);
    }

};

template <int trail_size>
class Trail
{
    int current_trail_size = 0;
    int end_of_trail = 0;
    vector<vec2> trail;
    QColor color;

public:
    Trail(QColor &c): color{c}
    {
    }

    void updateTrail(vec2 &new_point)
    {
        if(current_trail_size < trail_size)
        {
            trail.push_back(new_point);
            ++current_trail_size;
        }
        else
        {
            trail[end_of_trail] = new_point;
            ++end_of_trail;
            if(end_of_trail == trail_size)
                end_of_trail = 0;
        }
    }

    void drawTrail(QPainter *painter)
    {
        for(int i = 0, current_index = end_of_trail; i < current_trail_size - 1; ++i, ++current_index)
        {
            if(current_index == current_trail_size)
                current_index = 0;
            painter->setPen(QPen(QColor(color.red(),color.green(),color.blue(),255*((float)(i)/current_trail_size)),2));
            if(current_index + 1 == current_trail_size)
                painter->drawLine(trail[current_index].x,trail[current_index].y,trail[0].x,trail[0].y);
            else
                painter->drawLine(trail[current_index].x,trail[current_index].y,trail[current_index+1].x,trail[current_index+1].y);
        }
    }

};


float calculateMass(float radius)
{
    return (4*PI*pow(radius,3))/3;
}



class Body
{
    vec2 position;
    vec2 velocity;
    float mass;
    float radius;
    QColor color;

    Trail<400> trail;

public:
    Body(float r, QColor &&body_color, QColor &&trail_color, vec2 &&pos = vec2(0,0), vec2 &&vel = vec2(0,0)):
        position{pos}, velocity{vel}, radius{r}, color{body_color}, trail{trail_color}
    {
        mass = calculateMass(r);
    }

    void move()
    {
        trail.updateTrail(position);
        vec2 increament = velocity * time_coefficient;
        position += increament;
    }

    void draw(QPainter *painter)
    {
        trail.drawTrail(painter);

        painter->setPen(QPen(color));
        painter->setBrush(QBrush(QColor(color)));
        painter->drawEllipse(position.x - radius, position.y - radius, 2*radius, 2*radius);
    }

    void interact(Body &body)
    {
        float dx = body.position.x - position.x;
        float dy = body.position.y - position.y;
        float distance = sqrt(pow(dx,2) + pow(dy,2));
        if(distance == 0) return;

        float force = (G * mass * body.mass)/pow(distance,2);
        vec2 this_acceleration = vec2(dx*((force/mass)/distance),dy*((force/mass)/distance));
        vec2 body_acceleration = vec2(-dx*((force/body.mass)/distance),-dy*((force/body.mass)/distance));

        velocity += this_acceleration * time_coefficient;
        body.velocity += body_acceleration * time_coefficient;
    }

    vec2 getPosition()
    {
        return position;
    }

    vec2 getVelocity()
    {
        return velocity;
    }

    float getRadius()
    {
        return radius;
    }

    friend vec2 calculateAcceleration(Body &object_1, Body &object_2);

};



vec2 calculateAcceleration(Body &object_1, Body &object_2)
{
    float dx = object_2.position.x - object_1.position.x;
    float dy = object_2.position.y - object_1.position.y;
    float distance = sqrt(pow(dx,2) + pow(dy,2));
    if(distance == 0) return vec2(0,0);

    float force = (G * object_1.mass * object_2.mass)/pow(distance,2);
    vec2 acceleration = vec2(dx*((force/object_1.mass)/distance),dy*((force/object_1.mass)/distance));

    return acceleration;
}



class Camera
{
public:
    vec2 position;
    Body *target;

    Camera(vec2 &&pos, Body *targ = nullptr): position(pos), target(targ)
    {
    }

    void updatePosition()
    {
        if(target != nullptr)
            position = target->getPosition();
    }

    void updateTarget(Body *targ)
    {
        target = targ;
    }

};



class BodyAdder
{
    vec2 start_position;
    vec2 end_position;
    bool processing = false;
    float radius = 1;
    float mass = calculateMass(radius);
    list<Body> &planets;
    Camera &camera;


public:
    BodyAdder(list<Body> &p, Camera &cam): planets{p}, camera{cam}
    {
    }

    void setRadius(float r)
    {
        radius = r;
        mass = calculateMass(radius);
    }

    void setStartPosition(vec2 &&start_pos)
    {
        start_position.x = start_pos.x;
        start_position.y = start_pos.y;
        processing = true;
    }

    void setEndPosition(vec2 &&end_pos)
    {
        end_position.x = end_pos.x;
        end_position.y = end_pos.y;
    }

    void drawDirection(QPainter *painter)
    {
        const int number_of_iterations = 30;
        vec2 speed = calculateSpeed();
        vec2 old_position = start_position;
        vec2 current_position = start_position;

        float dif_x = camera.position.x - WIDTH/2;
        float dif_y = camera.position.y - HEIGHT/2;

        for(int i = 0; i < number_of_iterations; ++i)
        {
            current_position += speed;
            painter->setPen(QPen(QColor(255,255,255,255*((float)(i)/number_of_iterations)),2));
            painter->drawLine(old_position.x + dif_x, old_position.y + dif_y, current_position.x + dif_x, current_position.y + dif_y);

            vec2 acceleration = calculateFullAcceleration(Body(radius,QColor(),QColor(),current_position + vec2(dif_x,dif_y)));
            speed += acceleration;

            old_position = current_position;
        }
    }

    void addBody()
    {
        if(processing)
        {
            vec2 speed = calculateSpeed();
            if(camera.target != nullptr)
                 speed = speed + camera.target->getVelocity();

            int red;
            int green;
            int blue;
            int brightness;
            do
            {
                red = rand() % 255;
                green = rand() % 255;
                blue = rand() % 255;
                brightness = red + green + blue;
            }
            while (brightness < 255);

            vec2 body_position(start_position.x + camera.position.x - WIDTH/2 , start_position.y + camera.position.y - HEIGHT/2);
            planets.push_back(Body(radius,QColor(red,green,blue),QColor(red*0.6,green*0.6,blue*0.6),std::move(body_position),std::move(speed)));

            processing = false;
        }
    }

    bool isProcessing()
    {
        return processing;
    }

private:
    vec2 calculateSpeed()
    {
        float distance = sqrt(pow(end_position.x-start_position.x,2) + pow(end_position.y-start_position.y,2));
        float speed_coef = pow(distance,2)/1000000;
        vec2 speed(speed_coef*(end_position.x-start_position.x), speed_coef*(end_position.y-start_position.y));
        return speed;
    }

    vec2 calculateFullAcceleration(Body &&object)
    {
        vec2 result_acceleration(0,0);
        for(auto body = planets.begin(); body != planets.end(); ++body)
        {
            vec2 acceleration = calculateAcceleration(object,*body);
            result_acceleration += acceleration;
        }

        return result_acceleration;
    }

};



class MainWidget: public QWidget
{
    list<Body> planets;
    Camera camera{vec2((float)(WIDTH)/2,(float)(HEIGHT)/2)};
    BodyAdder body_adder{planets, camera};
    bool isPaused = false;


public:

    MainWidget(QWidget *parent = nullptr): QWidget(parent)
    {
        resize(WIDTH,HEIGHT);
        setWindowIcon(QIcon("solar-system.png"));
        setWindowTitle("Gravity Toy");
        startTimer(1000/120);

        QLabel *radius_label = new QLabel("RADIUS:\n    1",this);
        radius_label->setFont(QFont("OldEnglish",8));
        radius_label->setStyleSheet("QLabel{color: blue}");
        radius_label->setGeometry(12,235,50,50);


        QSlider *radius_changer = new QSlider(Qt::Vertical,this);
        radius_changer->setGeometry(25,25,18,200);
        QApplication::connect(radius_changer,&QSlider::valueChanged,this,[&,radius_label](int value)
        {
            if(value != 0)
            {
                radius_label->setText("RADIUS:\n    " + QString().setNum(value));
                body_adder.setRadius(value);
            }
        });


        QLabel *time_label = new QLabel("1X",this);
        time_label->setFont(QFont("OldEnglish",9));
        time_label->setStyleSheet("QLabel{color: blue}");
        time_label->setGeometry(25 + 18 + 50 + 90,25 + 18,50,50);

        QSlider *time_changer = new QSlider(Qt::Horizontal,this);
        time_changer->setGeometry(25 + 18 + 50, 25, 200, 18);
        time_changer->setValue(50);
        QApplication::connect(time_changer,&QSlider::valueChanged,this,[&,time_label](int value)
        {
            if(value != 0)
            {
                time_coefficient = (float)(value) / 50;
                time_label->setText(QString().setNum(time_coefficient) + "X");
            }
        });


        QPushButton *delete_planets = new QPushButton("CLEAR EVERYTHIG",this);
        delete_planets->setGeometry(WIDTH-175,25,150,45);
        delete_planets->setFont(QFont("OldEnglish",9));
        delete_planets->setStyleSheet("QPushButton{background: #ffaf00}");
        QApplication::connect(delete_planets,&QPushButton::clicked,this,[&]()
        {
            planets.clear();
            camera.updateTarget(nullptr);
        });


        QPushButton *pause_button = new QPushButton("PAUSE",this);
        pause_button->setIcon(QIcon("pause.png"));
        pause_button->setGeometry(WIDTH - 25 - 150 - 150 - 25, 25, 150, 45);
        pause_button->setFont(QFont("OldEnglish",9));
        pause_button->setStyleSheet("QPushButton{background: #ffffff}");
        QApplication::connect(pause_button,&QPushButton::clicked,this,[&,pause_button]()
        {
            if(!isPaused)
            {
                pause_button->setText("PLAY");
                pause_button->setIcon(QIcon("play.png"));
                isPaused = true;
            }
            else
            {
                pause_button->setText("PAUSE");
                pause_button->setIcon(QIcon("pause.png"));
                isPaused = false;
            }
        });


    }


    void mousePressEvent(QMouseEvent *event)
    {
        int x = event->x();
        int y = event->y();
        for(auto body = planets.begin(); body != planets.end(); ++body)
        {
            if( sqrt( pow((x+camera.position.x-WIDTH/2)-body->getPosition().x , 2) + pow((y+camera.position.y-HEIGHT/2)-body->getPosition().y , 2) ) < body->getRadius() )
            {
                camera.updateTarget(&(*body));
                return;
            }
        }

        body_adder.setStartPosition(vec2(x,y));
        body_adder.setEndPosition(vec2(x,y));
    }

    void mouseMoveEvent(QMouseEvent *event)
    {
        if(event->buttons() & Qt::LeftButton)
            body_adder.setEndPosition(vec2(event->x() , event->y()));
    }

    void mouseReleaseEvent(QMouseEvent *event)
    {
        Q_UNUSED(event)
        body_adder.addBody();
    }


    void paintEvent(QPaintEvent *event)
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.fillRect(0,0,WIDTH,HEIGHT,QBrush(0x000012));
        camera.updatePosition();
        painter.translate(-camera.position.x + WIDTH/2 , -camera.position.y + HEIGHT/2);
        painter.setRenderHint(QPainter::Antialiasing);


        for(auto planets_iterator = planets.begin(); planets_iterator != planets.end(); ++planets_iterator)
        {
            planets_iterator->draw(&painter);
            if(!isPaused)
            {
                planets_iterator->move();
                for(auto interaction_iterator = std::next(planets_iterator,1); interaction_iterator != planets.end(); ++interaction_iterator)
                {
                    interaction_iterator->interact(*planets_iterator);
                }
            }
        }

        if(body_adder.isProcessing())
            body_adder.drawDirection(&painter);

    }



    void timerEvent(QTimerEvent *event)
    {
        Q_UNUSED(event);

        repaint();
    }



};


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWidget w;
    w.show();
    return a.exec();
}
