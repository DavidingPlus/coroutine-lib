// 饿汉式单例。
class Singleton4
{

public:

    static Singleton4 *GetInstance();

    static void DeleteInstance();

    void print();


private:

    Singleton4();

    ~Singleton4();

    Singleton4(const Singleton4 &single);

    const Singleton4 &operator=(const Singleton4 &single);


private:

    static Singleton4 *m_singleton4;
};
