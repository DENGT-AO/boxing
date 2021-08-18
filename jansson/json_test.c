#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include <jansson.h>

int main(int argc, char **argv)
{
    json_t *obj = NULL;
    json_t *group = NULL;
    json_t *root = NULL;
    json_t *value = NULL;      //若在if，for等函数里面声明变量，仅在相关函数里面有用，跳出后就无用了。局部变量
    json_t *list = NULL;
    //FILE * fp = NULL;
    json_error_t  err;
    char *buf = NULL;
    int value1 = 0;
    const char *key;
    int ret = 0;

    //fp = fopen("jsondata.txt", "r+");
    //encoding
    obj = json_load_file("jsondata.txt", 0, &err);

    printf("%s", err.text);
    //assert(obj);
    if ( !obj )
    {
        fprintf( stderr, "err: on line %d: %s\n", err.line, err.text );
        return -1;
    }
    else
    {
        group = json_object_get( obj, "group" );
        assert( group );

        printf( "group:%s\n",  json_string_value( group ) );
        //decoding
        buf = json_dumps( obj, JSON_INDENT(4) );
        printf( "%s\n", buf );

        //packing
        root = json_pack( "{s:s, s:i}", "frute", "apple", "weight", 1 );
        list = json_array();
        assert( list );

        list = json_pack( "[ssb]", "foo", "bar", 1 );
        ret = json_object_set_new( root, "list", list );
        printf( "the size of list is %ld\n", json_array_size(list) );
        printf( "the list is %s\n", json_dumps(json_object_get( root, "list" ), JSON_INDENT(4)) );

        buf = NULL;
        buf = json_dumps( root, JSON_INDENT(4) ); 
        printf( "%s\n", buf );
        buf = NULL;

        //unpacking
        json_unpack( root, "{s:s, s:i}", "frute", &buf, "weight", &value1 );
        printf( "%s:%d\n", buf, value1 );
        //printf( "%s\n", err.text );
        
        //iter对对象中元素进行迭代，每一对形成键值对
        void *iter = json_object_iter( root );
        while ( iter )
        {
            key = json_object_iter_key( iter );
            value = json_object_iter_value( iter );
            switch( json_typeof( value ) )
            {
                case JSON_STRING:
                    printf( "%s:%s\n", key, json_string_value(value) );
                    break;
                case JSON_INTEGER:
                    json_unpack( value, "i", &value1 );
                    printf( "%s:%d\n", key, value1);
                    break;
            }
            iter = json_object_iter_next( root, iter );
        }
    }

    
    free( buf );
    json_decref( group );
    json_decref( value );
    json_decref( obj );
    json_decref( root );
    return 0;
}